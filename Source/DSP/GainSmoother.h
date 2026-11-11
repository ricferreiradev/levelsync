#pragma once

#include <algorithm>
#include <cmath>

namespace levelsync
{
// Attack = decreasing gain (more attenuation / less boost).
// Release = increasing gain (less attenuation / more boost).
// Times are one-pole time constants: 63.2% of a step in the specified time.
class GainSmoother
{
public:
    void prepare(double sampleRate) noexcept
    {
        rate = std::isfinite(sampleRate) && sampleRate >= 8000.0 ? sampleRate : 48000.0;
        currentDb = targetDb = 0.0;
        setTimes(250.0f, 700.0f);
    }

    void setTimes(float attackMs, float releaseMs) noexcept
    {
        attack = coefficient(attackMs, 250.0f, 50.0f, 1500.0f);
        release = coefficient(releaseMs, 700.0f, 100.0f, 3000.0f);
    }

    void setSilenceReturnTime(float milliseconds) noexcept
    {
        silenceReturn = coefficient(milliseconds, 1500.0f, 100.0f, 5000.0f);
    }

    void setMusicSilenceReturnTime(float milliseconds) noexcept
    {
        const auto time = std::isfinite(milliseconds) ? std::clamp(milliseconds, 50.0f, 3000.0f) : 300.0f;
        // This control is a transition duration rather than the controller's
        // one-pole time constant: reach 99.9% of unity within the selected time.
        musicSilenceReturn = std::exp(std::log(0.001) /
            (rate * static_cast<double>(time) * 0.001));
    }

    void setTarget(float db) noexcept
    {
        targetDb = std::isfinite(db) ? std::clamp(static_cast<double>(db), -12.0, 12.0) : 0.0;
        useSilenceReturn = false;
        useMusicSilenceReturn = false;
    }

    void setNeutralTarget() noexcept { targetDb = 0.0; useSilenceReturn = true; useMusicSilenceReturn = false; }
    void setMusicSilenceTarget() noexcept { targetDb = 0.0; useSilenceReturn = false; useMusicSilenceReturn = true; }

    float nextGain() noexcept
    {
        const double c = useMusicSilenceReturn ? musicSilenceReturn
                         : useSilenceReturn ? silenceReturn
                         : (targetDb < currentDb ? attack : release);
        currentDb = targetDb + c * (currentDb - targetDb);
        if (std::abs(currentDb - targetDb) < 1.0e-9)
            currentDb = targetDb;
        return static_cast<float>(std::pow(10.0, currentDb / 20.0));
    }

    float gainDb() const noexcept { return static_cast<float>(currentDb); }
    float targetGainDb() const noexcept { return static_cast<float>(targetDb); }

private:
    double coefficient(float ms, float fallback, float low, float high) const noexcept
    {
        const auto time = std::isfinite(ms) ? std::clamp(ms, low, high) : fallback;
        return std::exp(-1.0 / (rate * static_cast<double>(time) * 0.001));
    }

    double rate = 48000.0, attack = 0.0, release = 0.0, silenceReturn = 0.0, musicSilenceReturn = 0.0;
    double currentDb = 0.0, targetDb = 0.0;
    bool useSilenceReturn = false, useMusicSilenceReturn = false;
};
}
