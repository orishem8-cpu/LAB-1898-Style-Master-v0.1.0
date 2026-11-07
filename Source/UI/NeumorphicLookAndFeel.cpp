#include "NeumorphicLookAndFeel.h"

namespace lab1898
{
const juce::Colour Palette::background { 0xffe5e5e7 };
const juce::Colour Palette::panel { 0xffe9e9eb };
const juce::Colour Palette::ink { 0xff28292b };
const juce::Colour Palette::muted { 0xff77797d };
const juce::Colour Palette::orange { 0xffff941f };
const juce::Colour Palette::graph { 0xff252729 };

NeumorphicLookAndFeel::NeumorphicLookAndFeel()
{
    setColour(juce::Slider::textBoxTextColourId, Palette::ink);
    setColour(juce::Slider::textBoxBackgroundColourId, Palette::panel);
    setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour(juce::Label::textColourId, Palette::ink);
    setColour(juce::ComboBox::textColourId, Palette::ink);
    setColour(juce::ComboBox::backgroundColourId, Palette::panel);
    setColour(juce::ComboBox::outlineColourId, juce::Colours::transparentBlack);
    setColour(juce::PopupMenu::backgroundColourId, Palette::panel);
    setColour(juce::PopupMenu::textColourId, Palette::ink);
    setColour(juce::TextButton::buttonColourId, Palette::panel);
    setColour(juce::TextButton::textColourOffId, Palette::ink);
    setColour(juce::TextButton::textColourOnId, Palette::orange);
}

void NeumorphicLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h,
                                              float pos, float start, float end, juce::Slider&)
{
    const float size = (float) juce::jmin(w, h) * 0.72f;
    const float cx = x + w * 0.5f, cy = y + h * 0.46f;
    const juce::Rectangle<float> r(cx - size * 0.5f, cy - size * 0.5f, size, size);
    for (int i = 5; i >= 1; --i)
    {
        auto shadow = r.expanded((float) i * 1.5f).translated(0.0f, (float) i * 1.1f);
        g.setColour(juce::Colours::black.withAlpha(0.012f * (float) i));
        g.fillEllipse(shadow);
    }
    juce::ColourGradient body(juce::Colour(0xfffafafb), r.getX(), r.getY(),
                              juce::Colour(0xffd7d7da), r.getRight(), r.getBottom(), false);
    g.setGradientFill(body);
    g.fillEllipse(r);
    g.setColour(juce::Colours::white.withAlpha(0.88f));
    g.drawEllipse(r.reduced(1.0f), 1.2f);

    const float angle = start + pos * (end - start);
    juce::Path arc;
    arc.addCentredArc(cx, cy, size * 0.61f, size * 0.61f, 0.0f, start, angle, true);
    g.setColour(Palette::orange);
    g.strokePath(arc, juce::PathStrokeType(juce::jmax(2.0f, size * 0.035f), juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    juce::Path pointer;
    pointer.addRoundedRectangle(-1.7f, -size * 0.37f, 3.4f, size * 0.16f, 1.5f);
    g.setColour(Palette::ink);
    g.fillPath(pointer, juce::AffineTransform::rotation(angle).translated(cx, cy));
}

void NeumorphicLookAndFeel::drawLinearSlider(juce::Graphics& g, int x, int y, int w, int h,
                                              float pos, float, float, juce::Slider::SliderStyle style,
                                              juce::Slider&)
{
    if (style != juce::Slider::LinearVertical)
        return;
    const float trackX = x + w * 0.5f;
    const float top = y + 8.0f, bottom = y + h - 8.0f;
    juce::Rectangle<float> track(trackX - 4.0f, top, 8.0f, bottom - top);
    g.setColour(juce::Colour(0xffb8b9bc)); g.fillRoundedRectangle(track, 4.0f);
    g.setColour(juce::Colours::white.withAlpha(0.95f));
    g.drawRoundedRectangle(track.reduced(1.0f), 3.0f, 1.0f);
    juce::Rectangle<float> active(track.getX() + 1.5f, pos, track.getWidth() - 3.0f, bottom - pos);
    g.setColour(Palette::orange); g.fillRoundedRectangle(active, 2.0f);
    juce::Rectangle<float> thumb(trackX - 14.0f, pos - 11.0f, 28.0f, 22.0f);
    g.setColour(juce::Colours::black.withAlpha(0.10f)); g.fillRoundedRectangle(thumb.translated(0, 3), 10.0f);
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xffffffff), thumb.getTopLeft(), juce::Colour(0xffd2d3d5), thumb.getBottomRight(), false));
    g.fillRoundedRectangle(thumb, 10.0f);
    g.setColour(juce::Colours::white.withAlpha(0.9f)); g.drawRoundedRectangle(thumb, 10.0f, 1.0f);
}

void NeumorphicLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& b,
                                                  const juce::Colour&, bool highlighted, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced(1.0f);
    g.setColour(juce::Colours::black.withAlpha(down ? 0.035f : 0.07f));
    g.fillRoundedRectangle(r.translated(1.0f, 2.0f), 14.0f);
    g.setColour(highlighted || down ? juce::Colour(0xffdddddf) : Palette::panel);
    g.fillRoundedRectangle(r, 14.0f);
    g.setColour(juce::Colours::white.withAlpha(0.8f));
    g.drawRoundedRectangle(r, 14.0f, 1.0f);
}
}
