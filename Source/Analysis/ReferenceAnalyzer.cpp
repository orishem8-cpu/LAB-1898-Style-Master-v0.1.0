#include "ReferenceAnalyzer.h"
#include <cmath>

namespace lab1898
{
bool ReferenceAnalyzer::analyzeFile(const juce::File& file, ReferenceAnalysis& result, juce::String& error,
                                    const std::function<bool()>& shouldCancel)
{
    juce::AudioFormatManager formats;
    formats.registerBasicFormats();
    std::unique_ptr<juce::AudioFormatReader> reader(formats.createReaderFor(file));
    if (reader == nullptr)
    {
        error = "Unsupported or unreadable audio file";
        return false;
    }

    const auto rate = reader->sampleRate;
    if (rate < 8000.0 || reader->lengthInSamples <= 0)
    {
        error = "Invalid audio stream";
        return false;
    }

    // Cap analysis at 45 seconds: sufficient for tonal/loudness profiling, bounded memory and time.
    const auto count = (int64) juce::jmin((int64) reader->lengthInSamples, (int64) (rate * 45.0));
    constexpr int chunkSize = 8192;
    juce::AudioBuffer<float> chunk(1, chunkSize);
    double sumSquares = 0.0, lowSquares = 0.0, midSquares = 0.0, highSquares = 0.0;
    float peak = 0.0f;
    float lowState = 0.0f, midState = 0.0f;
    const float lp120 = 1.0f - std::exp(-2.0f * juce::MathConstants<float>::pi * 120.0f / (float) rate);
    const float highCut = juce::jmin(6000.0f, (float) rate * 0.42f);
    const float lp6000 = 1.0f - std::exp(-2.0f * juce::MathConstants<float>::pi * highCut / (float) rate);

    int64 position = 0;
    while (position < count)
    {
        if (shouldCancel && shouldCancel())
        {
            error = "Analysis cancelled";
            return false;
        }
        const int n = (int) juce::jmin((int64) chunkSize, count - position);
        chunk.clear();
        if (! reader->read(&chunk, 0, n, position, true, true))
        {
            error = "Could not read audio samples";
            return false;
        }
        const auto* data = chunk.getReadPointer(0);
        for (int i = 0; i < n; ++i)
        {
            const float x = std::isfinite(data[i]) ? data[i] : 0.0f;
            peak = juce::jmax(peak, std::abs(x));
            sumSquares += (double) x * x;
            lowState += lp120 * (x - lowState);
            midState += lp6000 * (x - midState);
            const float low = lowState;
            const float mid = midState - lowState;
            const float high = x - midState;
            lowSquares += (double) low * low;
            midSquares += (double) mid * mid;
            highSquares += (double) high * high;
        }
        position += n;
    }

    const double samples = (double) juce::jmax<int64>(1, count);
    const double rms = std::sqrt(sumSquares / samples);
    if (peak < 1.0e-6f)
    {
        error = "Reference audio is silent or near-silent";
        return false;
    }
    const double energy = lowSquares + midSquares + highSquares;
    const double safeEnergy = juce::jmax(1.0e-20, energy);
    result.rmsDb = (float) (20.0 * std::log10(juce::jmax(1.0e-9, rms)));
    result.peakDb = (float) (20.0 * std::log10(juce::jmax(1.0e-9f, peak)));
    result.lowShare = (float) (lowSquares / safeEnergy);
    result.midShare = (float) (midSquares / safeEnergy);
    result.highShare = (float) (highSquares / safeEnergy);
    result.analyzedSeconds = (double) count / rate;
    return true;
}
}
