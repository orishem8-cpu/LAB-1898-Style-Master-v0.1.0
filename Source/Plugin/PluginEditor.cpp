#include "PluginEditor.h"
#include <cmath>

namespace
{
using namespace juce;
const std::array<const char*, 13> parameterIds {{
    "intensity", "colour", "harmonics", "bass", "punch", "mid", "air", "width",
    "drive", "output", "bias", "tone", "mix"
}};
const std::array<const char*, 13> captions {{
    "INTENSITY", "COLOR", "HARMONICS", "BASS", "PUNCH", "MID", "AIR", "WIDTH",
    "DRIVE", "OUTPUT", "BIAS", "TONE", "MIX"
}};
bool isRotary(int i) { return i <= 2 || i >= 8; }
}

void SpectrumView::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    g.setColour(lab1898::Palette::graph);
    g.fillRoundedRectangle(bounds, 18.0f);
    g.setColour(juce::Colours::white.withAlpha(0.10f));
    for (int i = 1; i < 5; ++i)
    {
        const float y = bounds.getY() + bounds.getHeight() * (float) i / 5.0f;
        g.drawHorizontalLine((int) y, bounds.getX() + 12.0f, bounds.getRight() - 12.0f);
    }
    for (int i = 1; i < 8; ++i)
    {
        const float x = bounds.getX() + bounds.getWidth() * (float) i / 8.0f;
        g.drawVerticalLine((int) x, bounds.getY() + 8.0f, bounds.getBottom() - 8.0f);
    }

    std::array<float, 64> bands {};
    processor.copySpectrumTo(bands);
    juce::Path fill, line;
    const float plotLeft = bounds.getX() + 12.0f, plotRight = bounds.getRight() - 12.0f;
    const float plotTop = bounds.getY() + 10.0f, plotBottom = bounds.getBottom() - 12.0f;
    const float plotWidth = plotRight - plotLeft;
    fill.startNewSubPath(plotLeft, plotBottom);
    for (int i = 0; i < (int) bands.size(); ++i)
    {
        const float x = plotLeft + plotWidth * (float) i / (float) (bands.size() - 1);
        const float shaped = std::pow(juce::jlimit(0.0f, 1.0f, bands[(size_t) i]), 0.72f);
        const float y = plotBottom - shaped * (plotBottom - plotTop) * 0.83f;
        if (i == 0) line.startNewSubPath(x, y); else line.lineTo(x, y);
        fill.lineTo(x, y);
    }
    fill.lineTo(plotRight, plotBottom);
    fill.closeSubPath();
    g.setColour(juce::Colour(0xffaeb0b2).withAlpha(0.35f));
    g.fillPath(fill);
    g.setColour(lab1898::Palette::orange);
    g.strokePath(line, juce::PathStrokeType(2.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour(juce::Colours::white.withAlpha(0.65f));
    g.setFont(juce::Font(10.5f));
    g.drawText("20", (int) plotLeft, (int) plotBottom - 4, 30, 14, juce::Justification::left);
    g.drawText("100", (int) (plotLeft + plotWidth * 0.28f), (int) plotBottom - 4, 36, 14, juce::Justification::centred);
    g.drawText("1k", (int) (plotLeft + plotWidth * 0.57f), (int) plotBottom - 4, 30, 14, juce::Justification::centred);
    g.drawText("10k", (int) (plotLeft + plotWidth * 0.82f), (int) plotBottom - 4, 34, 14, juce::Justification::centred);
    g.drawText("20k", (int) plotRight - 32, (int) plotBottom - 4, 32, 14, juce::Justification::right);
}

LAB1898AudioProcessorEditor::LAB1898AudioProcessorEditor(LAB1898AudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p), spectrumView(p)
{
    setLookAndFeel(&lookAndFeel);
    setResizable(true, true);
    setResizeLimits(1120, 700, 1920, 1080);
    setSize(1600, 900);

    brand.setText("LAB 1898", dontSendNotification);
    brand.setFont(Font("Arial", 27.0f, Font::bold));
    brand.setColour(Label::textColourId, lab1898::Palette::ink);
    productName.setText("STYLE MASTER", dontSendNotification);
    productName.setFont(Font("Arial", 11.0f, Font::plain));
    productName.setColour(Label::textColourId, lab1898::Palette::muted);
    styleLabel.setText("STYLE", dontSendNotification);
    styleLabel.setFont(Font("Arial", 13.0f, Font::bold));
    styleLabel.setJustificationType(Justification::centred);
    const juce::StringArray styleNames { "RODEO — ATMOSPHERIC", "ASTRO — PSYCHEDELIC", "ASTRO — SCREW TEXTURE",
                                         "UTOPIA — CINEMATIC", "UTOPIA — DARK IMPACT", "MELODIC EMOTION",
                                         "DARK RAGE", "ANALOG WARMTH" };
    styleBoxHeader.addItemList(styleNames, 1);
    styleBoxMain.addItemList(styleNames, 1);
    styleHeaderAttachment = std::make_unique<LAB1898AudioProcessor::APVTS::ComboBoxAttachment>(processor.getParameters(), "style", styleBoxHeader);
    styleMainAttachment = std::make_unique<LAB1898AudioProcessor::APVTS::ComboBoxAttachment>(processor.getParameters(), "style", styleBoxMain);

    spectrumHeading.setText("SPECTRUM", dontSendNotification);
    spectrumHeading.setFont(Font("Arial", 13.0f, Font::bold));
    spectrumHeading.setColour(Label::textColourId, lab1898::Palette::ink);
    meterReading.setText("RMS  --.-     PEAK  --.- dBFS", dontSendNotification);
    meterReading.setFont(Font("Arial", 12.0f, Font::bold));
    meterReading.setColour(Label::textColourId, lab1898::Palette::ink);
    meterReading.setJustificationType(Justification::centredRight);

    addAndMakeVisible(brand); addAndMakeVisible(productName); addAndMakeVisible(styleLabel);
    addAndMakeVisible(spectrumHeading); addAndMakeVisible(meterReading);
    addAndMakeVisible(styleBoxHeader); addAndMakeVisible(styleBoxMain); addAndMakeVisible(spectrumView);
    addAndMakeVisible(aButton); addAndMakeVisible(bButton);
    addAndMakeVisible(referenceButton); addAndMakeVisible(autoButton);
    addAndMakeVisible(analogButton); addAndMakeVisible(warmthButton);
    addAndMakeVisible(bypassButton); addAndMakeVisible(powerButton);
    addAndMakeVisible(statusLabel);

    for (int i = 0; i < (int) sliders.size(); ++i)
        createSlider(i, parameterIds[(size_t) i], captions[(size_t) i], isRotary(i));

    analogAttachment = std::make_unique<LAB1898AudioProcessor::APVTS::ButtonAttachment>(processor.getParameters(), "analog", analogButton);
    warmthAttachment = std::make_unique<LAB1898AudioProcessor::APVTS::ButtonAttachment>(processor.getParameters(), "warmth", warmthButton);
    bypassAttachment = std::make_unique<LAB1898AudioProcessor::APVTS::ButtonAttachment>(processor.getParameters(), "bypass", bypassButton);
    powerAttachment = std::make_unique<LAB1898AudioProcessor::APVTS::ButtonAttachment>(processor.getParameters(), "power", powerButton);

    aButton.onClick = [this] { processor.switchAB(0); };
    bButton.onClick = [this] { processor.switchAB(1); };
    referenceButton.onClick = [this] { chooseReference(); };
    autoButton.onClick = [this]
    {
        processor.applyAutoMatch();
        statusLabel.setText(processor.hasReference() ? "STARTING POINT APPLIED — ADJUST BY EAR" : "LOAD A REFERENCE FIRST", dontSendNotification);
    };
    statusLabel.setFont(Font("Arial", 10.5f, Font::bold));
    statusLabel.setColour(Label::textColourId, lab1898::Palette::muted);
    statusLabel.setJustificationType(Justification::centredLeft);

    startTimerHz(20);
}

LAB1898AudioProcessorEditor::~LAB1898AudioProcessorEditor()
{
    stopTimer();
    for (auto& slider : sliders) slider.setLookAndFeel(nullptr);
    setLookAndFeel(nullptr);
}

void LAB1898AudioProcessorEditor::createSlider(int index, const juce::String& parameter,
                                               const juce::String& caption, bool rotary)
{
    auto& s = sliders[(size_t) index];
    auto& label = sliderLabels[(size_t) index];
    s.setSliderStyle(rotary ? Slider::RotaryHorizontalVerticalDrag : Slider::LinearVertical);
    s.setTextBoxStyle(rotary ? Slider::TextBoxBelow : Slider::TextBoxAbove, false, 66, 18);
    s.setRotaryParameters(juce::MathConstants<float>::pi * 1.25f,
                          juce::MathConstants<float>::pi * 2.75f, true);
    s.setMouseDragSensitivity(180);
    s.setScrollWheelEnabled(false);
    s.setLookAndFeel(&lookAndFeel);
    label.setText(caption, dontSendNotification);
    label.setFont(Font("Arial", 11.5f, Font::bold));
    label.setColour(Label::textColourId, lab1898::Palette::ink);
    label.setJustificationType(Justification::centred);
    addAndMakeVisible(s); addAndMakeVisible(label);
    sliderAttachments.push_back(std::make_unique<LAB1898AudioProcessor::APVTS::SliderAttachment>(processor.getParameters(), parameter, s));
}

void LAB1898AudioProcessorEditor::drawPanel(juce::Graphics& g, juce::Rectangle<float> r, float radius)
{
    for (int i = 7; i >= 1; --i)
    {
        g.setColour(juce::Colours::black.withAlpha(0.012f * (float) i));
        g.fillRoundedRectangle(r.translated(0.0f, (float) i * 1.25f), radius);
    }
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xfff1f1f2), r.getTopLeft(),
                                           juce::Colour(0xffdedee1), r.getBottomRight(), false));
    g.fillRoundedRectangle(r, radius);
    g.setColour(juce::Colours::white.withAlpha(0.8f));
    g.drawRoundedRectangle(r.reduced(0.8f), radius, 1.1f);
}

void LAB1898AudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(lab1898::Palette::background);
    const auto w = (float) getWidth(), h = (float) getHeight();
    drawPanel(g, { 24.0f, 18.0f, w - 48.0f, 74.0f }, 36.0f);
    // Small waveform mark echoing the supplied LAB 1898 reference logo.
    g.setColour(lab1898::Palette::ink);
    const float logoX = 54.0f, logoY = 54.0f;
    const std::array<float, 9> bars {{ 9.0f, 18.0f, 27.0f, 15.0f, 32.0f, 20.0f, 12.0f, 23.0f, 10.0f }};
    for (size_t i = 0; i < bars.size(); ++i)
        g.fillRoundedRectangle(logoX + (float) i * 5.0f, logoY - bars[i] * 0.5f, 2.4f, bars[i], 1.2f);
    const float bodyY = 110.0f, bodyH = h - bodyY - 22.0f;
    const float gap = 18.0f, leftW = (w - 48.0f - 2.0f * gap) * 0.29f;
    const float centerW = (w - 48.0f - 2.0f * gap) * 0.42f;
    const float rightW = w - 48.0f - 2.0f * gap - leftW - centerW;
    drawPanel(g, { 24.0f, bodyY, leftW, bodyH }, 30.0f);
    drawPanel(g, { 24.0f + leftW + gap, bodyY, centerW, bodyH }, 30.0f);
    drawPanel(g, { 24.0f + leftW + gap + centerW + gap, bodyY, rightW, bodyH }, 30.0f);
    g.setColour(juce::Colour(0xffb8b9bc).withAlpha(0.65f));
    const float leftX = 24.0f, centerX = 24.0f + leftW + gap;
    g.drawHorizontalLine((int) (bodyY + bodyH * 0.67f), leftX + 42.0f, leftX + leftW - 42.0f);
    g.drawHorizontalLine((int) (bodyY + bodyH * 0.86f), leftX + 42.0f, leftX + leftW - 42.0f);
    g.drawHorizontalLine((int) (bodyY + bodyH * 0.87f), centerX + 34.0f, centerX + centerW - 34.0f);
}

void LAB1898AudioProcessorEditor::resized()
{
    const float w = (float) getWidth(), h = (float) getHeight();
    const float bodyY = 110.0f, bodyH = h - bodyY - 22.0f;
    const float gap = 18.0f, leftW = (w - 48.0f - 2.0f * gap) * 0.29f;
    const float centerW = (w - 48.0f - 2.0f * gap) * 0.42f;
    const float rightW = w - 48.0f - 2.0f * gap - leftW - centerW;
    const float leftX = 24.0f, centerX = leftX + leftW + gap, rightX = centerX + centerW + gap;

    brand.setBounds(82, 28, 235, 34);
    productName.setBounds(84, 60, 170, 17);
    styleBoxHeader.setBounds((int) (w * 0.35f), 35, 350, 42);
    aButton.setBounds((int) (w * 0.35f + 360), 35, 48, 42);
    bButton.setBounds((int) (w * 0.35f + 412), 35, 48, 42);

    styleLabel.setBounds((int) leftX + 30, (int) bodyY + 22, (int) leftW - 60, 22);
    const float leftInnerX = leftX + 30.0f, leftInnerW = leftW - 60.0f;
    styleBoxMain.setBounds((int) leftInnerX, (int) (bodyY + 48), (int) leftInnerW, 42);
    spectrumHeading.setBounds((int) centerX + 30, (int) bodyY + 22, 160, 22);
    meterReading.setBounds((int) (centerX + centerW - 245), (int) bodyY + 22, 215, 22);
    sliders[0].setBounds((int) (leftX + leftW * 0.18f), (int) (bodyY + 106), (int) (leftW * 0.64f), (int) (bodyH * 0.30f));
    sliderLabels[0].setBounds((int) leftX + 25, (int) (bodyY + bodyH * 0.47f), (int) leftW - 50, 20);
    const float smallW = leftInnerW * 0.47f;
    sliders[1].setBounds((int) leftInnerX, (int) (bodyY + bodyH * 0.54f), (int) smallW, (int) (bodyH * 0.23f));
    sliders[2].setBounds((int) (leftInnerX + smallW + 6), (int) (bodyY + bodyH * 0.54f), (int) smallW, (int) (bodyH * 0.23f));
    sliderLabels[1].setBounds((int) leftInnerX, (int) (bodyY + bodyH * 0.77f), (int) smallW, 19);
    sliderLabels[2].setBounds((int) (leftInnerX + smallW + 6), (int) (bodyY + bodyH * 0.77f), (int) smallW, 19);
    analogButton.setBounds((int) (leftX + 40), (int) (bodyY + bodyH * 0.90f), 130, 28);
    warmthButton.setBounds((int) (leftX + leftW * 0.53f), (int) (bodyY + bodyH * 0.90f), 130, 28);

    const float centerPad = 32.0f;
    const float graphY = bodyY + 64.0f, graphH = bodyH * 0.38f;
    spectrumView.setBounds((int) (centerX + 22), (int) graphY, (int) (centerW - 44), (int) graphH);
    const float sliderAreaY = bodyY + bodyH * 0.55f;
    const float colW = (centerW - 2.0f * centerPad) / 5.0f;
    for (int j = 0; j < 5; ++j)
    {
        const int index = 3 + j;
        const float x = centerX + centerPad + j * colW;
        sliderLabels[(size_t) index].setBounds((int) x, (int) sliderAreaY, (int) colW, 22);
        sliders[(size_t) index].setBounds((int) x + (int) (colW * 0.2f), (int) sliderAreaY + 25, (int) (colW * 0.6f), (int) (bodyH * 0.30f));
    }
    referenceButton.setBounds((int) (centerX + 34), (int) (bodyY + bodyH - 70), (int) (centerW * 0.60f), 46);
    autoButton.setBounds((int) (centerX + centerW - 145), (int) (bodyY + bodyH - 70), 112, 46);
    statusLabel.setBounds((int) (centerX + 34), (int) (bodyY + bodyH - 22), (int) centerW - 68, 16);

    const float knobW = (rightW - 72.0f) * 0.5f;
    const float topKnobY = bodyY + 62.0f;
    const float row2Y = bodyY + bodyH * 0.39f;
    const float x1 = rightX + 24.0f, x2 = x1 + knobW + 24.0f;
    const float bigH = bodyH * 0.23f;
    for (int k = 0; k < 4; ++k)
    {
        const int index = 8 + k;
        const float x = (k % 2 == 0) ? x1 : x2;
        const float y = (k < 2) ? topKnobY : row2Y;
        sliderLabels[(size_t) index].setBounds((int) x, (int) y, (int) knobW, 20);
        sliders[(size_t) index].setBounds((int) x, (int) y + 18, (int) knobW, (int) bigH);
    }
    const float mixX = rightX + rightW * 0.30f, mixW = rightW * 0.40f;
    sliderLabels[12].setBounds((int) mixX, (int) (bodyY + bodyH * 0.68f), (int) mixW, 20);
    sliders[12].setBounds((int) mixX, (int) (bodyY + bodyH * 0.70f), (int) mixW, (int) (bodyH * 0.20f));
    bypassButton.setBounds((int) (rightX + 40), (int) (bodyY + bodyH - 48), 128, 28);
    powerButton.setBounds((int) (rightX + rightW - 142), (int) (bodyY + bodyH - 48), 112, 28);
}

void LAB1898AudioProcessorEditor::chooseReference()
{
    fileChooser = std::make_unique<juce::FileChooser>("Choose a reference audio file",
        juce::File::getSpecialLocation(juce::File::userMusicDirectory), "*.wav;*.aif;*.aiff;*.flac");
    fileChooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [safeThis = juce::Component::SafePointer<LAB1898AudioProcessorEditor>(this)](const juce::FileChooser& chooser)
        {
            if (safeThis == nullptr) return;
            const auto file = chooser.getResult();
            if (file.existsAsFile())
            {
                safeThis->processor.analyzeReferenceAsync(file);
                safeThis->statusLabel.setText("ANALYZING REFERENCE IN BACKGROUND...", juce::dontSendNotification);
            }
        });
}

void LAB1898AudioProcessorEditor::timerCallback()
{
    spectrumView.repaint();
    meterReading.setText("RMS " + juce::String(processor.getOutputRmsDb(), 1) + "   PEAK "
                         + juce::String(processor.getOutputPeakDb(), 1) + " dBFS", juce::dontSendNotification);
    if (processor.isReferenceBusy())
        statusLabel.setText("ANALYZING REFERENCE...", juce::dontSendNotification);
    else if (processor.referenceAnalysisFailed())
        statusLabel.setText("REFERENCE COULD NOT BE ANALYZED — CHECK FILE FORMAT", juce::dontSendNotification);
    else if (processor.hasReference())
    {
        const auto r = processor.getReferenceAnalysis();
        statusLabel.setText("REFERENCE READY  |  RMS " + juce::String(r.rmsDb, 1) + " dBFS  |  PEAK "
                            + juce::String(r.peakDb, 1) + " dBFS  |  " + juce::String(r.analyzedSeconds, 0) + " s", juce::dontSendNotification);
    }
}
