#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace levelsync
{
// Detects sidechain silence from unweighted, channel-averaged mean-square
// energy. The reported value is dBFS energy: 10*log10((L^2 + R^2) / N).
// This is intentionally separate from the K-weighted loudness estimate.
class MusicSilenceDetector
{
public:
    struct Settings
    {
        bool enabled = true;
        float thresholdDbfs = -55.0f;
        float holdMs = 200.0f;
        float releaseMs = 400.0f;
    };

    void prepare(double newSampleRate) noexcept
    {
        sampleRate = std::isfinite(newSampleRate) && newSampleRate >= 8000.0
                         ? newSampleRate : 48000.0;
        // A short 5 ms energy envelope follows stops promptly. The explicit
        // hold and hysteresis below provide the anti-chatter protection.
        energyCoefficient = std::exp(-1.0 / (sampleRate * 0.005));
        smoothedEnergy = 0.0;
        belowSamples = aboveSamples = 0;
        musicActive = true; // Preserve normal startup behaviour until silence is confirmed.
        levelDbfs = -120.0f;
        setSettings(Settings{});
    }

    void setSettings(const Settings& value) noexcept
    {
        settings.enabled = value.enabled;
        settings.thresholdDbfs = std::isfinite(value.thresholdDbfs)
                                     ? std::clamp(value.thresholdDbfs, -90.0f, -20.0f) : -55.0f;
        settings.holdMs = std::isfinite(value.holdMs)
                              ? std::clamp(value.holdMs, 0.0f, 2000.0f) : 200.0f;
        settings.releaseMs = std::isfinite(value.releaseMs)
                                 ? std::clamp(value.releaseMs, 0.0f, 3000.0f) : 400.0f;
    }

    void push(const std::array<float, 2>& samples, bool sidechainPresent) noexcept
    {
        if (!sidechainPresent)
        {
            smoothedEnergy = 0.0;
            levelDbfs = -120.0f;
            belowSamples = aboveSamples = 0;
            musicActive = false;
            return;
        }

        double energy = 0.0;
        for (float sample : samples)
        {
            const double x = std::isfinite(sample) ? sample : 0.0;
            energy += x * x;
        }
        energy *= 0.5; // Channel-average energy keeps the dBFS scale independent of channel count.
        smoothedEnergy = energy + energyCoefficient * (smoothedEnergy - energy);
        levelDbfs = static_cast<float>(10.0 * std::log10(std::max(smoothedEnergy, 1.0e-12)));

        if (!settings.enabled)
        {
            musicActive = true;
            belowSamples = aboveSamples = 0;
            return;
        }

        const auto holdSamples = millisecondsToSamples(settings.holdMs);
        const auto releaseSamples = millisecondsToSamples(settings.releaseMs);
        if (musicActive)
        {
            aboveSamples = 0;
            belowSamples = levelDbfs < settings.thresholdDbfs ? belowSamples + 1 : 0;
            if (belowSamples >= holdSamples)
            {
                musicActive = false;
                belowSamples = 0;
            }
        }
        else
        {
            belowSamples = 0;
            // 3 dB hysteresis: music must clear a higher boundary before protection releases.
            aboveSamples = levelDbfs > settings.thresholdDbfs + 3.0f ? aboveSamples + 1 : 0;
            if (aboveSamples >= releaseSamples)
            {
                musicActive = true;
                aboveSamples = 0;
            }
        }
    }

    bool isMusicActive() const noexcept { return musicActive; }
    bool hasMusicSignal() const noexcept
    {
        return levelDbfs >= settings.thresholdDbfs;
    }
    bool isProtectionActive(bool sidechainPresent) const noexcept
    {
        return settings.enabled && sidechainPresent && !musicActive;
    }
    float getLevelDbfs() const noexcept { return levelDbfs; }

private:
    std::uint64_t millisecondsToSamples(float milliseconds) const noexcept
    {
        return std::max<std::uint64_t>(1, static_cast<std::uint64_t>(
            std::llround(sampleRate * static_cast<double>(milliseconds) * 0.001)));
    }

    Settings settings;
    double sampleRate = 48000.0, energyCoefficient = 0.0, smoothedEnergy = 0.0;
    std::uint64_t belowSamples = 0, aboveSamples = 0;
    float levelDbfs = -120.0f;
    bool musicActive = true;
};
}
