#pragma once
#include <JuceHeader.h>
#include <array>

namespace lab1898
{
struct Parameters
{
    int style = 0;
    float intensity = 72.0f;
    float colour = 48.0f;
    float harmonics = 63.0f;
    float bass = 2.0f;
    float punch = 1.5f;
    float mid = 0.0f;
    float air = 1.0f;
    float width = 115.0f;
    float drive = 52.0f;
    float outputDb = 0.0f;
    float bias = 48.0f;
    float tone = 60.0f;
    float mix = 100.0f;
    bool analog = true;
    bool warmth = false;
    bool bypass = false;
    bool power = true;
};

class Engine
{
public:
    void prepare(double sampleRate, int maximumBlockSize, int channels);
    void reset() noexcept;
    void processBlock(juce::AudioBuffer<float>&, const Parameters&) noexcept;

private:
    struct Biquad
    {
        float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f, a1 = 0.0f, a2 = 0.0f;
        float z1 = 0.0f, z2 = 0.0f;
        void set(float nb0, float nb1, float nb2, float na1, float na2) noexcept;
        float process(float x) noexcept;
        void reset() noexcept { z1 = z2 = 0.0f; }
    };

    struct Smoothers
    {
        juce::SmoothedValue<float> intensity, colour, harmonics, bass, punch, mid, air,
                                   width, drive, outputGain, bias, tone, mix, bypassMix,
                                   styleLow, styleMid, styleHigh, styleDrive, analog, warmth;
    } sm;

    static void makePeak(Biquad&, double sr, float hz, float q, float gainDb) noexcept;
    static void makeLowShelf(Biquad&, double sr, float hz, float gainDb) noexcept;
    static void makeHighShelf(Biquad&, double sr, float hz, float gainDb) noexcept;

    double sampleRate = 44100.0;
    float envelope = 0.0f;
    float attackCoeff = 0.0f, releaseCoeff = 0.0f;
    bool eqCoefficientsValid = false;
    float lastLowGain = 999.0f, lastMidGain = 999.0f, lastHighGain = 999.0f;
    std::array<std::array<Biquad, 3>, 2> eq {};
};
}
