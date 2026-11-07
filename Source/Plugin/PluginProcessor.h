#pragma once
#include <JuceHeader.h>
#include "../DSP/Engine.h"
#include "../Analysis/ReferenceAnalyzer.h"
#include <array>
#include <atomic>
#include <memory>

class LAB1898AudioProcessor final : public juce::AudioProcessor
{
public:
    using APVTS = juce::AudioProcessorValueTreeState;
    LAB1898AudioProcessor();
    ~LAB1898AudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "LAB 1898 Style Master"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    APVTS& getParameters() noexcept { return parameters; }
    void analyzeReferenceAsync(const juce::File& file);
    bool isReferenceBusy() const noexcept { return referenceBusy.load(std::memory_order_acquire); }
    bool hasReference() const noexcept { return referenceReady.load(std::memory_order_acquire); }
    bool referenceAnalysisFailed() const noexcept { return referenceFailed.load(std::memory_order_acquire); }
    lab1898::ReferenceAnalysis getReferenceAnalysis() const noexcept;
    void applyAutoMatch();
    void switchAB(int slot);
    void copySpectrumTo(std::array<float, 64>& destination) const noexcept;
    float getOutputRmsDb() const noexcept { return outputRmsDb.load(std::memory_order_relaxed); }
    float getOutputPeakDb() const noexcept { return outputPeakDb.load(std::memory_order_relaxed); }
    static APVTS::ParameterLayout createParameterLayout();

private:
    friend class ReferenceTask;
    void setParameter(const juce::String& id, float value);
    void updateSpectrum(const juce::AudioBuffer<float>& buffer) noexcept;

    APVTS parameters;
    lab1898::Engine engine;
    juce::ThreadPool referencePool { 1 };
    std::atomic<bool> referenceBusy { false }, referenceReady { false }, referenceFailed { false };
    mutable juce::CriticalSection referenceDataLock;
    lab1898::ReferenceAnalysis referenceData {};
    juce::CriticalSection stateLock;
    std::array<juce::ValueTree, 2> abStates {};
    std::array<bool, 2> abHasState {{ false, false }};

    static constexpr int fftOrder = 11;
    static constexpr int fftSize = 1 << fftOrder;
    juce::dsp::FFT fft { fftOrder };
    std::array<float, fftSize> fftFifo {};
    std::array<float, fftSize> fftWindow {};
    std::array<float, fftSize * 2> fftWork {};
    std::array<int, 64> fftBandFirst {}, fftBandLast {};
    int fftWritePosition = 0;
    std::array<std::atomic<float>, 64> spectrum {};
    std::atomic<float> outputRmsDb { -120.0f }, outputPeakDb { -120.0f };
    float rmsMeterLinear = 0.0f, peakMeterLinear = 0.0f; // audio-thread only
    std::array<std::atomic<float>*, 18> parameterCache {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LAB1898AudioProcessor)
};
