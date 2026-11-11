#pragma once

#include "../DSP/TelemetryQueue.h"
#include "LevelSyncLookAndFeel.h"
#include <array>

namespace levelsync::ui
{
class LoudnessMeter final : public juce::Component
{
public:
    LoudnessMeter(juce::String title, juce::Colour colour)
        : name(std::move(title)), meterColour(colour) {}
    void setLevel(float newDb, bool newValid, bool newActive) noexcept;
    void paint(juce::Graphics&) override;

private:
    juce::String name;
    juce::Colour meterColour;
    float displayedDb = -70.0f;
    bool valid = false, active = false;
};

class BipolarGainMeter final : public juce::Component
{
public:
    void setGain(float db, bool boostLimit, bool cutLimit) noexcept;
    void paint(juce::Graphics&) override;

private:
    float displayedDb = 0.0f;
    bool boostLimited = false, cutLimited = false;
};

class BalanceHistory final : public juce::Component
{
public:
    static constexpr std::size_t capacity = 750; // 30 s at 25 Hz.
    void push(const TelemetryFrame&) noexcept;
    void clear() noexcept { count = write = 0; repaint(); }
    void paint(juce::Graphics&) override;

private:
    std::array<TelemetryFrame, capacity> frames{};
    std::size_t write = 0, count = 0;
};

class SpectralGraph final : public juce::Component
{
public:
    void setFrame(const TelemetryFrame& newFrame) noexcept { frame = newFrame; repaint(); }
    void paint(juce::Graphics&) override;
private:
    TelemetryFrame frame;
};
}
