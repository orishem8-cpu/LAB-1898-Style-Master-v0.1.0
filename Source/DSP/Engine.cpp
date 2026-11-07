#include "Engine.h"
#include <cmath>

namespace lab1898
{
namespace
{
float dbToGain(float db) noexcept { return std::pow(10.0f, db / 20.0f); }
float clampFinite(float x, float lo, float hi) noexcept
{
    return std::isfinite(x) ? juce::jlimit(lo, hi, x) : lo;
}
}

void Engine::Biquad::set(float nb0, float nb1, float nb2, float na1, float na2) noexcept
{
    b0 = nb0; b1 = nb1; b2 = nb2; a1 = na1; a2 = na2;
}

float Engine::Biquad::process(float x) noexcept
{
    const float y = b0 * x + z1;
    z1 = b1 * x - a1 * y + z2;
    z2 = b2 * x - a2 * y;
    if (! std::isfinite(y) || ! std::isfinite(z1) || ! std::isfinite(z2))
        reset();
    return std::isfinite(y) ? y : 0.0f;
}

void Engine::makePeak(Biquad& f, double sr, float hz, float q, float gainDb) noexcept
{
    const double w = 2.0 * juce::MathConstants<double>::pi * juce::jlimit(20.0f, (float) (sr * 0.45), hz) / sr;
    const double a = std::pow(10.0, gainDb / 40.0);
    const double alpha = std::sin(w) / (2.0 * juce::jmax(0.15f, q));
    const double a0 = 1.0 + alpha / a;
    f.set((float) ((1.0 + alpha * a) / a0), (float) (-2.0 * std::cos(w) / a0),
          (float) ((1.0 - alpha * a) / a0), (float) (-2.0 * std::cos(w) / a0),
          (float) ((1.0 - alpha / a) / a0));
}

void Engine::makeLowShelf(Biquad& f, double sr, float hz, float gainDb) noexcept
{
    const double w = 2.0 * juce::MathConstants<double>::pi * juce::jlimit(20.0f, (float) (sr * 0.40), hz) / sr;
    const double a = std::pow(10.0, gainDb / 40.0), c = std::cos(w), s = std::sin(w);
    const double alpha = s * 0.70710678;
    const double two = 2.0 * std::sqrt(a) * alpha;
    const double a0 = (a + 1.0) + (a - 1.0) * c + two;
    f.set((float) (a * ((a + 1.0) - (a - 1.0) * c + two) / a0),
          (float) (2.0 * a * ((a - 1.0) - (a + 1.0) * c) / a0),
          (float) (a * ((a + 1.0) - (a - 1.0) * c - two) / a0),
          (float) (-2.0 * ((a - 1.0) + (a + 1.0) * c) / a0),
          (float) (((a + 1.0) + (a - 1.0) * c - two) / a0));
}

void Engine::makeHighShelf(Biquad& f, double sr, float hz, float gainDb) noexcept
{
    const double w = 2.0 * juce::MathConstants<double>::pi * juce::jlimit(1000.0f, (float) (sr * 0.45), hz) / sr;
    const double a = std::pow(10.0, gainDb / 40.0), c = std::cos(w), s = std::sin(w);
    const double alpha = s * 0.70710678;
    const double two = 2.0 * std::sqrt(a) * alpha;
    const double a0 = (a + 1.0) - (a - 1.0) * c + two;
    f.set((float) (a * ((a + 1.0) + (a - 1.0) * c + two) / a0),
          (float) (-2.0 * a * ((a - 1.0) + (a + 1.0) * c) / a0),
          (float) (a * ((a + 1.0) + (a - 1.0) * c - two) / a0),
          (float) (2.0 * ((a - 1.0) - (a + 1.0) * c) / a0),
          (float) (((a + 1.0) - (a - 1.0) * c - two) / a0));
}

void Engine::prepare(double sr, int, int)
{
    sampleRate = juce::jmax(8000.0, sr);
    const auto ramp = 0.035;
    const auto init = [ramp, this](auto& s, float v)
    {
        s.reset(sampleRate, ramp);
        s.setCurrentAndTargetValue(v);
    };
    init(sm.intensity, 72.0f); init(sm.colour, 48.0f); init(sm.harmonics, 63.0f);
    init(sm.bass, 2.0f); init(sm.punch, 1.5f); init(sm.mid, 0.0f); init(sm.air, 1.0f);
    init(sm.width, 115.0f); init(sm.drive, 52.0f); init(sm.outputGain, 1.0f);
    init(sm.bias, 48.0f); init(sm.tone, 60.0f); init(sm.mix, 100.0f); init(sm.bypassMix, 0.0f);
    init(sm.styleLow, 0.25f); init(sm.styleMid, 0.10f); init(sm.styleHigh, 0.35f); init(sm.styleDrive, 0.0f);
    init(sm.analog, 1.0f); init(sm.warmth, 0.0f);
    attackCoeff = std::exp(-1.0f / (0.010f * (float) sampleRate));
    releaseCoeff = std::exp(-1.0f / (0.120f * (float) sampleRate));
    reset();
}

void Engine::reset() noexcept
{
    envelope = 0.0f;
    eqCoefficientsValid = false;
    for (auto& channel : eq)
        for (auto& filter : channel)
            filter.reset();
}

void Engine::processBlock(juce::AudioBuffer<float>& buffer, const Parameters& p) noexcept
{
    const int channels = juce::jmin(2, buffer.getNumChannels());
    const int samples = buffer.getNumSamples();
    if (channels == 0 || samples == 0) return;

    sm.intensity.setTargetValue(clampFinite(p.intensity, 0.0f, 100.0f));
    sm.colour.setTargetValue(clampFinite(p.colour, 0.0f, 100.0f));
    sm.harmonics.setTargetValue(clampFinite(p.harmonics, 0.0f, 100.0f));
    sm.bass.setTargetValue(clampFinite(p.bass, -9.0f, 9.0f));
    sm.punch.setTargetValue(clampFinite(p.punch, -6.0f, 6.0f));
    sm.mid.setTargetValue(clampFinite(p.mid, -9.0f, 9.0f));
    sm.air.setTargetValue(clampFinite(p.air, -9.0f, 9.0f));
    sm.width.setTargetValue(clampFinite(p.width, 0.0f, 150.0f));
    sm.drive.setTargetValue(clampFinite(p.drive, 0.0f, 100.0f));
    sm.outputGain.setTargetValue(dbToGain(clampFinite(p.outputDb, -18.0f, 12.0f)));
    sm.bias.setTargetValue(clampFinite(p.bias, 0.0f, 100.0f));
    sm.tone.setTargetValue(clampFinite(p.tone, 0.0f, 100.0f));
    sm.mix.setTargetValue(clampFinite(p.mix, 0.0f, 100.0f));
    sm.analog.setTargetValue(p.analog ? 1.0f : 0.0f);
    sm.warmth.setTargetValue(p.warmth ? 1.0f : 0.0f);
    const bool bypassed = p.bypass || ! p.power;
    sm.bypassMix.setTargetValue(bypassed ? 1.0f : 0.0f);

    // Each named style adds a restrained fixed voicing on top of the user's controls.
    static constexpr float styleLow[]   = { 0.25f, -0.15f, 0.65f, 0.15f, 1.0f, 0.10f, 0.45f, 0.35f };
    static constexpr float styleMid[]   = { 0.10f, 0.65f, 0.20f, 0.25f, 0.45f, 0.45f, 1.0f, -0.25f };
    static constexpr float styleHigh[]  = { 0.35f, 0.50f, -0.35f, 0.75f, -0.65f, 0.35f, 0.15f, -0.90f };
    static constexpr float styleDrive[] = { 0.0f, 0.15f, 0.35f, 0.05f, 0.25f, 0.0f, 0.55f, 0.10f };
    const int styleIndex = juce::jlimit(0, 7, p.style);
    sm.styleLow.setTargetValue(styleLow[styleIndex]);
    sm.styleMid.setTargetValue(styleMid[styleIndex]);
    sm.styleHigh.setTargetValue(styleHigh[styleIndex]);
    sm.styleDrive.setTargetValue(styleDrive[styleIndex]);

    // EQ controls are smoothed in sample time, then coefficients are updated once per block.
    const float eqColour = sm.colour.getCurrentValue();
    const float eqBass = sm.bass.getCurrentValue();
    const float eqMid = sm.mid.getCurrentValue();
    const float eqAir = sm.air.getCurrentValue();
    const float eqTone = sm.tone.getCurrentValue();
    const float eqPunch = sm.punch.getCurrentValue();
    const float warmthAmount = sm.warmth.getCurrentValue();
    const float voicingLow = sm.styleLow.getCurrentValue();
    const float voicingMid = sm.styleMid.getCurrentValue();
    const float voicingHigh = sm.styleHigh.getCurrentValue();
    const float colourTilt = (eqColour - 50.0f) * 0.035f;
    const float warmthLift = -0.8f * warmthAmount;
    const float lowGain = eqBass + warmthLift + voicingLow;
    const float midGain = eqMid + voicingMid + (eqTone - 50.0f) * 0.025f;
    const float highGain = eqAir + voicingHigh + colourTilt;
    if (! eqCoefficientsValid || lowGain != lastLowGain || midGain != lastMidGain || highGain != lastHighGain)
    {
        for (int ch = 0; ch < channels; ++ch)
        {
            makeLowShelf(eq[(size_t) ch][0], sampleRate, 105.0f, lowGain);
            makePeak(eq[(size_t) ch][1], sampleRate, 900.0f, 0.85f, midGain);
            makeHighShelf(eq[(size_t) ch][2], sampleRate, 8500.0f, highGain);
        }
        lastLowGain = lowGain; lastMidGain = midGain; lastHighGain = highGain;
        eqCoefficientsValid = true;
    }

    auto* left = buffer.getWritePointer(0);
    auto* right = channels > 1 ? buffer.getWritePointer(1) : nullptr;
    const float threshold = dbToGain(-18.0f + eqPunch * 0.55f);

    for (int i = 0; i < samples; ++i)
    {
        const float intensity = sm.intensity.getNextValue() * 0.01f;
        const float harmonic = sm.harmonics.getNextValue() * 0.01f;
        const float drive = sm.drive.getNextValue() * 0.01f;
        const float bias = (sm.bias.getNextValue() - 50.0f) * 0.0025f;
        const float analog = sm.analog.getNextValue();
        const float mix = sm.mix.getNextValue() * 0.01f;
        const float bypassBlend = sm.bypassMix.getNextValue();
        const float width = sm.width.getNextValue() * 0.01f;
        const float output = sm.outputGain.getNextValue();
        const float dryL = left[i];
        const float dryR = right != nullptr ? right[i] : dryL;

        float l = eq[0][2].process(eq[0][1].process(eq[0][0].process(dryL)));
        float r = right != nullptr ? eq[1][2].process(eq[1][1].process(eq[1][0].process(dryR))) : l;

        const float driveGain = 1.0f + drive * 2.4f + sm.styleDrive.getNextValue();
        const float satAmount = (0.04f + analog * 0.16f) + harmonic * 0.30f;
        const float biasDrive = bias * driveGain;
        const float biasOffset = std::tanh(biasDrive);
        const float smallSignalGain = juce::jmax(0.10f, driveGain * (1.0f - biasOffset * biasOffset));
        const float harmonicGain = 1.0f + satAmount * 2.0f;
        const auto saturate = [driveGain, bias, biasOffset, smallSignalGain, satAmount, harmonicGain](float x) noexcept
        {
            // Bias is DC-compensated and the local slope is normalised to avoid an unwanted level jump.
            const float driven = (std::tanh((x + bias) * driveGain) - biasOffset) / smallSignalGain;
            const float harmonicLayer = std::tanh(x * harmonicGain) / harmonicGain;
            const float blend = satAmount * 0.18f;
            return juce::jlimit(-1.25f, 1.25f, driven * (1.0f - blend) + harmonicLayer * blend);
        };
        l = saturate(l);
        r = saturate(r);

        if (right != nullptr)
        {
            const float mid = 0.5f * (l + r);
            const float side = 0.5f * (l - r) * juce::jlimit(0.0f, 1.5f, width);
            l = mid + side;
            r = mid - side;
        }

        const float detector = juce::jmax(std::abs(l), std::abs(r));
        const float coeff = detector > envelope ? attackCoeff : releaseCoeff;
        envelope = coeff * envelope + (1.0f - coeff) * detector;
        const float ratio = 1.0f + intensity * 1.65f;
        float compGain = 1.0f;
        if (envelope > threshold)
        {
            const float compressed = threshold + (envelope - threshold) / ratio;
            compGain = compressed / juce::jmax(0.0001f, envelope);
        }
        // Intensity blends the compressor in gradually rather than switching it abruptly.
        compGain = 1.0f + (compGain - 1.0f) * intensity;
        l *= compGain;
        r *= compGain;

        // A restrained output soft-knee stage protects against abrupt overs.
        const auto safety = [](float x) noexcept
        {
            constexpr float knee = 0.94f;
            if (std::abs(x) <= knee) return x;
            const float sign = x < 0.0f ? -1.0f : 1.0f;
            const float excess = std::abs(x) - knee;
            return sign * (knee + 0.06f * std::tanh(excess / 0.06f));
        };
        l = safety(l * output);
        r = safety(r * output);
        const float processedL = dryL + (l - dryL) * mix;
        const float processedR = dryR + (r - dryR) * mix;
        // Smooth dry/processed transition avoids a discontinuity when Bypass or Power is toggled.
        left[i] = dryL + (processedL - dryL) * (1.0f - bypassBlend);
        if (right != nullptr) right[i] = dryR + (processedR - dryR) * (1.0f - bypassBlend);
    }
    sm.colour.skip(samples); sm.bass.skip(samples); sm.mid.skip(samples);
    sm.air.skip(samples); sm.tone.skip(samples); sm.punch.skip(samples);
    sm.styleLow.skip(samples); sm.styleMid.skip(samples); sm.styleHigh.skip(samples);
    sm.warmth.skip(samples);
}
}
