#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "../UI/NeumorphicLookAndFeel.h"
#include <array>
#include <memory>
#include <vector>

class SpectrumView final : public juce::Component
{
public:
    explicit SpectrumView(const LAB1898AudioProcessor& p) : processor(p) {}
    void paint(juce::Graphics&) override;
private:
    const LAB1898AudioProcessor& processor;
};

class LAB1898AudioProcessorEditor final : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit LAB1898AudioProcessorEditor(LAB1898AudioProcessor&);
    ~LAB1898AudioProcessorEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void createSlider(int index, const juce::String& parameter, const juce::String& caption, bool rotary);
    void drawPanel(juce::Graphics&, juce::Rectangle<float>, float radius = 28.0f);
    void chooseReference();

    LAB1898AudioProcessor& processor;
    lab1898::NeumorphicLookAndFeel lookAndFeel;
    juce::Label brand, productName, styleLabel, statusLabel, spectrumHeading, meterReading;
    juce::ComboBox styleBoxHeader, styleBoxMain;
    juce::TextButton aButton { "A" }, bButton { "B" }, referenceButton { "REFERENCE MATCH" }, autoButton { "AUTO" };
    juce::ToggleButton analogButton { "ANALOG" }, warmthButton { "WARMTH" }, bypassButton { "BYPASS" }, powerButton { "POWER" };
    SpectrumView spectrumView;
    std::array<juce::Slider, 13> sliders;
    std::array<juce::Label, 13> sliderLabels;
    std::vector<std::unique_ptr<LAB1898AudioProcessor::APVTS::SliderAttachment>> sliderAttachments;
    std::unique_ptr<LAB1898AudioProcessor::APVTS::ComboBoxAttachment> styleHeaderAttachment, styleMainAttachment;
    std::unique_ptr<LAB1898AudioProcessor::APVTS::ButtonAttachment> analogAttachment, warmthAttachment, bypassAttachment, powerAttachment;
    std::unique_ptr<juce::FileChooser> fileChooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LAB1898AudioProcessorEditor)
};
