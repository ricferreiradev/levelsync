#pragma once

#include <algorithm>
#include <array>
#include <cmath>

namespace levelsync
{
// Energy gate only: cannot reliably distinguish speech from spill or other sounds.
class VocalActivityDetector
{
public:
    struct Settings
    {
        float thresholdDb = -50.0f;
        float hysteresisDb = 6.0f;
        float minimumActivityMs = 80.0f;
        float holdMs = 250.0f;
    };

    void prepare(double sampleRate) noexcept
    {
        rate = std::isfinite(sampleRate) && sampleRate >= 8000.0 ? sampleRate : 48000.0;
        smoothing = std::exp(-1.0 / (rate * 0.01));
        energy = 0.0;
        aboveSamples = belowSamples = 0;
        active = reliable = false;
        setSettings(Settings{});
    }

    void setSettings(const Settings& settings) noexcept
    {
        const auto safe = [](float x, float fallback, float low, float high)
        { return std::isfinite(x) ? std::clamp(x, low, high) : fallback; };
        const auto threshold = safe(settings.thresholdDb, -50.0f, -80.0f, -20.0f);
        const auto hysteresis = safe(settings.hysteresisDb, 6.0f, 0.0f, 24.0f);
        onEnergy = std::pow(10.0, threshold / 10.0);
        offEnergy = std::pow(10.0, (threshold - hysteresis) / 10.0);
        minimumSamples = std::max(1, static_cast<int>(rate * 0.001
                            * safe(settings.minimumActivityMs, 80.0f, 1.0f, 1000.0f)));
        holdSamples = std::max(1, static_cast<int>(rate * 0.001
                            * safe(settings.holdMs, 250.0f, 1.0f, 2000.0f)));
    }

    void push(const std::array<float, 2>& samples, int channels) noexcept
    {
        double power = 0.0;
        bool finite = true;
        const int count = std::clamp(channels, 1, 2);
        for (int ch = 0; ch < count; ++ch)
        {
            const double x = samples[static_cast<std::size_t>(ch)];
            const bool valid = std::isfinite(x) && std::abs(x) <= 1.0e6;
            finite = finite && valid;
            if (valid)
                power += x * x;
        }
        energy = smoothing * energy + (1.0 - smoothing) * power / count;
        if (!active)
        {
            aboveSamples = finite && energy >= onEnergy ? aboveSamples + 1 : 0;
            if (aboveSamples >= minimumSamples)
            {
                active = true;
                aboveSamples = belowSamples = 0;
            }
        }
        else if (!finite || energy < offEnergy)
        {
            if (++belowSamples >= holdSamples)
            {
                active = false;
                belowSamples = 0;
            }
        }
        else
            belowSamples = 0;

        // Hold the activity indication between phrases, but never chase residual noise.
        reliable = active && finite && energy >= offEnergy;
    }

    bool isActive() const noexcept { return active; }
    bool isReliable() const noexcept { return reliable; }

private:
    double rate = 48000.0, smoothing = 0.0, energy = 0.0;
    double onEnergy = 0.0, offEnergy = 0.0;
    int aboveSamples = 0, belowSamples = 0, minimumSamples = 1, holdSamples = 1;
    bool active = false, reliable = false;
};
}
