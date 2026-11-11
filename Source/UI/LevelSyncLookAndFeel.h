#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace levelsync::ui
{
struct Colours
{
    static constexpr juce::uint32 background = 0xff11161d;
    static constexpr juce::uint32 panel = 0xff1a222c;
    static constexpr juce::uint32 panelRaised = 0xff212c37;
    static constexpr juce::uint32 outline = 0xff344250;
    static constexpr juce::uint32 text = 0xffeef4f7;
    static constexpr juce::uint32 muted = 0xff8fa2b3;
    static constexpr juce::uint32 vocal = 0xff36d6b1;
    static constexpr juce::uint32 music = 0xff469ee8;
    static constexpr juce::uint32 correction = 0xffffa43a;
    static constexpr juce::uint32 danger = 0xffff6262;
};

class LevelSyncLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    LevelSyncLookAndFeel()
    {
        setColour(juce::Slider::textBoxTextColourId, juce::Colour(Colours::text));
        setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(Colours::background));
        setColour(juce::Slider::textBoxOutlineColourId, juce::Colour(Colours::outline));
        setColour(juce::Label::textColourId, juce::Colour(Colours::text));
        setColour(juce::TextButton::textColourOffId, juce::Colour(Colours::text));
        setColour(juce::TextButton::textColourOnId, juce::Colour(Colours::text));
        setColour(juce::ProgressBar::backgroundColourId, juce::Colour(Colours::background));
        setColour(juce::ProgressBar::foregroundColourId, juce::Colour(Colours::vocal));
    }

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float startAngle, float endAngle,
                          juce::Slider&) override
    {
        auto bounds = juce::Rectangle<float>(static_cast<float>(x), static_cast<float>(y),
                                             static_cast<float>(width), static_cast<float>(height))
                          .reduced(7.0f);
        const float radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.5f;
        const auto centre = bounds.getCentre();
        const auto angle = startAngle + sliderPos * (endAngle - startAngle);
        juce::Path track;
        track.addCentredArc(centre.x, centre.y, radius, radius, 0.0f,
                            startAngle, endAngle, true);
        g.setColour(juce::Colour(Colours::outline));
        g.strokePath(track, juce::PathStrokeType(5.0f, juce::PathStrokeType::curved,
                                                 juce::PathStrokeType::rounded));
        juce::Path value;
        value.addCentredArc(centre.x, centre.y, radius, radius, 0.0f,
                            startAngle, angle, true);
        g.setColour(juce::Colour(Colours::vocal));
        g.strokePath(value, juce::PathStrokeType(5.0f, juce::PathStrokeType::curved,
                                                 juce::PathStrokeType::rounded));
        const auto pointer = juce::Point<float>(centre.x + std::sin(angle) * (radius - 3.0f),
                                                centre.y - std::cos(angle) * (radius - 3.0f));
        g.fillEllipse(pointer.x - 3.0f, pointer.y - 3.0f, 6.0f, 6.0f);
    }

    void drawButtonBackground(juce::Graphics& g, juce::Button& button,
                              const juce::Colour&, bool highlighted, bool down) override
    {
        auto colour = juce::Colour(button.getToggleState() ? Colours::vocal : Colours::panelRaised);
        if (highlighted) colour = colour.brighter(0.12f);
        if (down) colour = colour.darker(0.12f);
        if (!button.isEnabled()) colour = colour.withAlpha(0.4f);
        g.setColour(colour);
        g.fillRoundedRectangle(button.getLocalBounds().toFloat(), 6.0f);
        g.setColour(juce::Colour(Colours::outline));
        g.drawRoundedRectangle(button.getLocalBounds().toFloat().reduced(0.5f), 6.0f, 1.0f);
    }
};
}
