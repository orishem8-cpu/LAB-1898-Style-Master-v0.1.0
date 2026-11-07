#pragma once
#include <JuceHeader.h>

namespace lab1898
{
struct Palette
{
    static const juce::Colour background;
    static const juce::Colour panel;
    static const juce::Colour ink;
    static const juce::Colour muted;
    static const juce::Colour orange;
    static const juce::Colour graph;
};

class NeumorphicLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    NeumorphicLookAndFeel();
    void drawRotarySlider(juce::Graphics&, int x, int y, int width, int height,
                          float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                          juce::Slider&) override;
    void drawLinearSlider(juce::Graphics&, int x, int y, int width, int height,
                          float sliderPos, float minSliderPos, float maxSliderPos,
                          juce::Slider::SliderStyle, juce::Slider&) override;
    void drawButtonBackground(juce::Graphics&, juce::Button&, const juce::Colour&,
                              bool highlighted, bool down) override;
};
}
