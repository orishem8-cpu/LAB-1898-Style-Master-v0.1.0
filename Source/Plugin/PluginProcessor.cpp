#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>
#include <algorithm>

namespace
{
using namespace juce;
using APVTS = LAB1898AudioProcessor::APVTS;

std::unique_ptr<AudioParameterFloat> makeFloat(const char* id, const char* name,
                                               float min, float max, float initial, const char* suffix = "")
{
    return std::make_unique<AudioParameterFloat>(ParameterID { id, 1 }, name,
        NormalisableRange<float>(min, max, 0.01f), initial, AudioParameterFloatAttributes().withLabel(suffix));
}
std::unique_ptr<AudioParameterBool> makeBool(const char* id, const char* name, bool initial)
{
    return std::make_unique<AudioParameterBool>(ParameterID { id, 1 }, name, initial);
}
std::unique_ptr<AudioParameterChoice> makeChoice(const char* id, const char* name,
                                                 const StringArray& choices, int initial)
{
    return std::make_unique<AudioParameterChoice>(ParameterID { id, 1 }, name, choices, initial);
}
}

class ReferenceTask final : public juce::ThreadPoolJob
{
public:
    ReferenceTask(LAB1898AudioProcessor& p, juce::File f)
        : ThreadPoolJob("LAB 1898 reference analysis"), processor(p), file(std::move(f)) {}

    JobStatus runJob() override
    {
        lab1898::ReferenceAnalysis result;
        juce::String error;
        const bool ok = lab1898::ReferenceAnalyzer::analyzeFile(file, result, error, [this] { return shouldExit(); });
        if (shouldExit())
        {
            processor.referenceBusy.store(false, std::memory_order_release);
            return jobHasFinished;
        }
        if (ok)
        {
            {
                const juce::ScopedLock lock(processor.referenceDataLock);
                processor.referenceData = result;
            }
            processor.referenceFailed.store(false, std::memory_order_relaxed);
            processor.referenceReady.store(true, std::memory_order_release);
        }
        else
        {
            processor.referenceFailed.store(true, std::memory_order_release);
        }
        processor.referenceBusy.store(false, std::memory_order_release);
        return jobHasFinished;
    }
private:
    LAB1898AudioProcessor& processor;
    juce::File file;
};

LAB1898AudioProcessor::APVTS::ParameterLayout LAB1898AudioProcessor::createParameterLayout()
{
    APVTS::ParameterLayout layout;
    layout.add(makeChoice("style", "Style", {
        "RODEO ATMOSPHERIC", "ASTRO PSYCHEDELIC", "ASTRO SCREW TEXTURE",
        "UTOPIA CINEMATIC", "UTOPIA DARK IMPACT", "MELODIC EMOTION",
        "DARK RAGE", "ANALOG WARMTH" }, 0));
    layout.add(makeFloat("intensity", "Intensity", 0.0f, 100.0f, 72.0f, "%"));
    layout.add(makeFloat("colour", "Color", 0.0f, 100.0f, 48.0f, "%"));
    layout.add(makeFloat("harmonics", "Harmonics", 0.0f, 100.0f, 63.0f, "%"));
    layout.add(makeFloat("bass", "Bass", -9.0f, 9.0f, 2.0f, "dB"));
    layout.add(makeFloat("punch", "Punch", -6.0f, 6.0f, 1.5f, "dB"));
    layout.add(makeFloat("mid", "Mid", -9.0f, 9.0f, 0.0f, "dB"));
    layout.add(makeFloat("air", "Air", -9.0f, 9.0f, 1.0f, "dB"));
    layout.add(makeFloat("width", "Width", 0.0f, 150.0f, 115.0f, "%"));
    layout.add(makeFloat("drive", "Drive", 0.0f, 100.0f, 52.0f, "%"));
    layout.add(makeFloat("output", "Output", -18.0f, 12.0f, 0.0f, "dB"));
    layout.add(makeFloat("bias", "Bias", 0.0f, 100.0f, 48.0f, "%"));
    layout.add(makeFloat("tone", "Tone", 0.0f, 100.0f, 60.0f, "%"));
    layout.add(makeFloat("mix", "Mix", 0.0f, 100.0f, 100.0f, "%"));
    layout.add(makeBool("analog", "Analog", true));
    layout.add(makeBool("warmth", "Warmth", false));
    layout.add(makeBool("bypass", "Bypass", false));
    layout.add(makeBool("power", "Power", true));
    return layout;
}

LAB1898AudioProcessor::LAB1898AudioProcessor()
    : AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true)
                                      .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, "LAB1898_STATE", createParameterLayout())
{
    constexpr const char* ids[] = { "style", "intensity", "colour", "harmonics", "bass", "punch", "mid", "air", "width", "drive", "output", "bias", "tone", "mix", "analog", "warmth", "bypass", "power" };
    for (size_t i = 0; i < parameterCache.size(); ++i)
        parameterCache[i] = parameters.getRawParameterValue(ids[i]);
    for (auto& band : spectrum) band.store(0.0f, std::memory_order_relaxed);
}

LAB1898AudioProcessor::~LAB1898AudioProcessor()
{
    referencePool.removeAllJobs(true, -1);
}

void LAB1898AudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    engine.prepare(sampleRate, samplesPerBlock, getTotalNumOutputChannels());
    fftWritePosition = 0;
    fftFifo.fill(0.0f);
    fftWork.fill(0.0f);
    const float nyquist = (float) sampleRate * 0.5f;
    for (int k = 0; k < fftSize; ++k)
        fftWindow[(size_t) k] = 0.5f - 0.5f * std::cos(2.0f * juce::MathConstants<float>::pi * (float) k / (float) (fftSize - 1));
    for (int band = 0; band < 64; ++band)
    {
        const float a = 25.0f * std::pow(20000.0f / 25.0f, (float) band / 64.0f);
        const float b = 25.0f * std::pow(20000.0f / 25.0f, (float) (band + 1) / 64.0f);
        fftBandFirst[(size_t) band] = juce::jlimit(1, fftSize / 2 - 1, (int) (a / nyquist * (fftSize / 2)));
        fftBandLast[(size_t) band] = juce::jlimit(fftBandFirst[(size_t) band] + 1, fftSize / 2,
                                                  (int) (b / nyquist * (fftSize / 2)));
    }
    rmsMeterLinear = peakMeterLinear = 0.0f;
    outputRmsDb.store(-120.0f, std::memory_order_relaxed);
    outputPeakDb.store(-120.0f, std::memory_order_relaxed);
}

void LAB1898AudioProcessor::releaseResources() {}

bool LAB1898AudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto& mainIn = layouts.getMainInputChannelSet();
    const auto& mainOut = layouts.getMainOutputChannelSet();
    return mainIn == mainOut && (mainOut == juce::AudioChannelSet::mono() || mainOut == juce::AudioChannelSet::stereo());
}

void LAB1898AudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    for (int ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear(ch, 0, buffer.getNumSamples());

    const auto read = [this](size_t index) noexcept
    {
        const auto* value = parameterCache[index];
        return value != nullptr ? value->load(std::memory_order_relaxed) : 0.0f;
    };
    lab1898::Parameters p;
    p.style = juce::jlimit(0, 7, (int) read(0));
    p.intensity = read(1); p.colour = read(2); p.harmonics = read(3);
    p.bass = read(4); p.punch = read(5); p.mid = read(6); p.air = read(7);
    p.width = read(8); p.drive = read(9); p.outputDb = read(10);
    p.bias = read(11); p.tone = read(12); p.mix = read(13);
    p.analog = read(14) > 0.5f; p.warmth = read(15) > 0.5f;
    p.bypass = read(16) > 0.5f; p.power = read(17) > 0.5f;
    engine.processBlock(buffer, p);
    updateSpectrum(buffer);
}

void LAB1898AudioProcessor::updateSpectrum(const juce::AudioBuffer<float>& buffer) noexcept
{
    const int chans = juce::jmin(2, buffer.getNumChannels());
    if (chans == 0) return;
    const int n = buffer.getNumSamples();
    const auto* l = buffer.getReadPointer(0);
    const auto* r = chans > 1 ? buffer.getReadPointer(1) : l;
    double sumSquares = 0.0;
    float blockPeak = 0.0f;
    for (int i = 0; i < n; ++i)
    {
        const float ml = std::isfinite(l[i]) ? l[i] : 0.0f;
        const float mr = std::isfinite(r[i]) ? r[i] : 0.0f;
        sumSquares += (double) ml * ml + (chans > 1 ? (double) mr * mr : 0.0);
        blockPeak = juce::jmax(blockPeak, juce::jmax(std::abs(ml), std::abs(mr)));
        fftFifo[(size_t) fftWritePosition++] = 0.5f * (ml + mr);
        if (fftWritePosition == fftSize)
        {
            for (int k = 0; k < fftSize; ++k)
                fftWork[(size_t) k] = fftFifo[(size_t) k] * fftWindow[(size_t) k];
            std::fill(fftWork.begin() + fftSize, fftWork.end(), 0.0f);
            fft.performFrequencyOnlyForwardTransform(fftWork.data());
            for (int band = 0; band < 64; ++band)
            {
                float bandPeak = 0.0f;
                const int first = fftBandFirst[(size_t) band];
                const int last = fftBandLast[(size_t) band];
                for (int bin = first; bin < last; ++bin)
                    bandPeak = juce::jmax(bandPeak, fftWork[(size_t) bin]);
                const float db = juce::Decibels::gainToDecibels(bandPeak / (float) fftSize, -72.0f);
                spectrum[(size_t) band].store(juce::jlimit(0.0f, 1.0f, (db + 72.0f) / 72.0f), std::memory_order_relaxed);
            }
            fftWritePosition = 0;
        }
    }
    const double divisor = (double) juce::jmax(1, n * chans);
    const float rms = (float) std::sqrt(sumSquares / divisor);
    const float blockSeconds = (float) n / (float) juce::jmax(1.0, getSampleRate());
    const float rmsTau = rms > rmsMeterLinear ? 0.055f : 0.320f;
    const float rmsCoeff = std::exp(-blockSeconds / rmsTau);
    rmsMeterLinear = rmsCoeff * rmsMeterLinear + (1.0f - rmsCoeff) * rms;
    peakMeterLinear = juce::jmax(blockPeak, peakMeterLinear * std::exp(-blockSeconds / 0.85f));
    outputRmsDb.store(juce::Decibels::gainToDecibels(rmsMeterLinear, -120.0f), std::memory_order_relaxed);
    outputPeakDb.store(juce::Decibels::gainToDecibels(peakMeterLinear, -120.0f), std::memory_order_relaxed);
}

lab1898::ReferenceAnalysis LAB1898AudioProcessor::getReferenceAnalysis() const noexcept
{
    const juce::ScopedLock lock(referenceDataLock);
    return referenceData;
}

void LAB1898AudioProcessor::copySpectrumTo(std::array<float, 64>& destination) const noexcept
{
    for (size_t i = 0; i < destination.size(); ++i)
        destination[i] = spectrum[i].load(std::memory_order_relaxed);
}

void LAB1898AudioProcessor::analyzeReferenceAsync(const juce::File& file)
{
    if (referenceBusy.exchange(true, std::memory_order_acq_rel)) return;
    referenceReady.store(false, std::memory_order_release);
    referenceFailed.store(false, std::memory_order_release);
    referencePool.addJob(new ReferenceTask(*this, file), true);
}

void LAB1898AudioProcessor::setParameter(const juce::String& id, float value)
{
    if (auto* parameter = parameters.getParameter(id))
        parameter->setValueNotifyingHost(parameter->convertTo0to1(value));
}

void LAB1898AudioProcessor::applyAutoMatch()
{
    if (! hasReference()) return;
    const auto ref = referenceData;
    // Gentle tonal correction only; this is a starting point, not a promise of exact mastering match.
    const float bassDelta = juce::jlimit(-2.0f, 2.0f, (0.34f - ref.lowShare) * 7.0f);
    const float airDelta = juce::jlimit(-2.0f, 2.0f, (0.33f - ref.highShare) * 8.0f);
    const float midDelta = juce::jlimit(-1.5f, 1.5f, (0.34f - ref.midShare) * 5.0f);
    setParameter("bass", bassDelta);
    setParameter("mid", midDelta);
    setParameter("air", airDelta);
    setParameter("intensity", 52.0f);
    setParameter("mix", 70.0f);
}

void LAB1898AudioProcessor::switchAB(int slot)
{
    const juce::ScopedLock lock(stateLock);
    slot = juce::jlimit(0, 1, slot);
    if (abHasState[(size_t) slot])
        parameters.replaceState(abStates[(size_t) slot].createCopy());
    else
    {
        abStates[(size_t) slot] = parameters.copyState();
        abHasState[(size_t) slot] = true;
    }
}

void LAB1898AudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    const juce::ScopedLock lock(stateLock);
    auto state = parameters.copyState();
    if (abHasState[0]) state.setProperty("abA", abStates[0].toXmlString(), nullptr);
    if (abHasState[1]) state.setProperty("abB", abStates[1].toXmlString(), nullptr);
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void LAB1898AudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    const juce::ScopedLock lock(stateLock);
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName(parameters.state.getType()))
    {
        auto state = juce::ValueTree::fromXml(*xml);
        parameters.replaceState(state);
        const auto a = state.getProperty("abA").toString();
        const auto b = state.getProperty("abB").toString();
        if (a.isNotEmpty())
            if (auto e = juce::XmlDocument::parse(a)) { abStates[0] = juce::ValueTree::fromXml(*e); abHasState[0] = true; }
        if (b.isNotEmpty())
            if (auto e = juce::XmlDocument::parse(b)) { abStates[1] = juce::ValueTree::fromXml(*e); abHasState[1] = true; }
    }
}

juce::AudioProcessorEditor* LAB1898AudioProcessor::createEditor()
{
    return new LAB1898AudioProcessorEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new LAB1898AudioProcessor();
}
