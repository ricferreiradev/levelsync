#pragma once

#include "LoudnessDetector.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace levelsync
{
enum class LearnState : std::uint8_t
{
    Idle,
    Preparing,
    Learning,
    Completed,
    Failed,
    Cancelled
};

enum class LearnFailure : std::uint8_t
{
    None,
    TimedOut
};

struct LearnSnapshot
{
    LearnState state = LearnState::Idle;
    LearnFailure failure = LearnFailure::None;
    float progress = 0.0f;
    float validSeconds = 0.0f;
    float referenceDb = 3.0f;
    bool hasValidReference = false;
};

// Collects one synchronised observation per control tick. A fixed histogram
// produces a time-weighted median without allocation, sorting, or locks on the
// audio thread. The median rejects isolated level outliers more effectively
// than an arithmetic mean of dB observations.
class ReferenceLearner
{
public:
    static constexpr float requiredValidSeconds = 5.0f;
    static constexpr float maximumSessionSeconds = 30.0f;
    static constexpr float unityToleranceDb = 0.01f;

    void prepare(float controlPeriodSeconds) noexcept
    {
        period = std::isfinite(controlPeriodSeconds)
                     ? std::clamp(controlPeriodSeconds, 0.001f, 0.1f) : 0.01f;
        resetSession();
        snapshot.state = LearnState::Idle;
    }

    void setStoredReference(float referenceDb, bool valid) noexcept
    {
        if (valid && std::isfinite(referenceDb))
        {
            snapshot.referenceDb = std::clamp(referenceDb, minimumDb, maximumDb);
            snapshot.hasValidReference = true;
        }
        else
        {
            snapshot.referenceDb = 3.0f;
            snapshot.hasValidReference = false;
        }
    }

    void start() noexcept
    {
        resetSession();
        snapshot.state = LearnState::Preparing;
    }

    void cancel() noexcept
    {
        if (snapshot.state == LearnState::Preparing || snapshot.state == LearnState::Learning)
        {
            snapshot.state = LearnState::Cancelled;
            snapshot.progress = 0.0f;
            snapshot.validSeconds = 0.0f;
        }
    }

    void acknowledgeCompletion() noexcept
    {
        if (snapshot.state == LearnState::Completed)
            snapshot.state = LearnState::Idle;
    }

    void process(float appliedGainDb, LoudnessMeasurement voice, LoudnessMeasurement music,
                 bool reliableVoiceActivity, bool sidechainValid) noexcept
    {
        if (snapshot.state != LearnState::Preparing && snapshot.state != LearnState::Learning)
            return;

        elapsedSeconds += period;
        if (elapsedSeconds >= maximumSessionSeconds)
        {
            snapshot.state = LearnState::Failed;
            snapshot.failure = LearnFailure::TimedOut;
            snapshot.progress = 0.0f;
            snapshot.validSeconds = 0.0f;
            return;
        }

        if (snapshot.state == LearnState::Preparing)
        {
            if (std::isfinite(appliedGainDb) && std::abs(appliedGainDb) <= unityToleranceDb)
            {
                ++unityTicks;
                if (unityTicks >= static_cast<std::uint32_t>(std::ceil(0.1f / period)))
                    snapshot.state = LearnState::Learning;
            }
            else
                unityTicks = 0;
            return;
        }

        if (!reliableVoiceActivity || !sidechainValid || !voice.reliable || !music.reliable
            || !std::isfinite(voice.db) || !std::isfinite(music.db))
            return;

        const float difference = voice.db - music.db;
        if (!std::isfinite(difference) || difference < minimumDb || difference > maximumDb)
            return;

        const auto bin = static_cast<std::size_t>(std::clamp(
            static_cast<int>(std::lround((difference - minimumDb) / binWidth)),
            0, static_cast<int>(histogram.size() - 1)));
        ++histogram[bin];
        ++validTicks;
        snapshot.validSeconds = validTicks * period;
        snapshot.progress = std::clamp(snapshot.validSeconds / requiredValidSeconds, 0.0f, 1.0f);

        const auto requiredTicks = static_cast<std::uint32_t>(std::ceil(requiredValidSeconds / period));
        if (validTicks >= requiredTicks)
            complete();
    }

    bool isActive() const noexcept
    {
        return snapshot.state == LearnState::Preparing || snapshot.state == LearnState::Learning;
    }

    LearnSnapshot getSnapshot() const noexcept { return snapshot; }

private:
    static constexpr float minimumDb = -12.0f;
    static constexpr float maximumDb = 12.0f;
    static constexpr float binWidth = 0.1f;
    static constexpr std::size_t binCount = 241;

    void resetSession() noexcept
    {
        histogram.fill(0);
        validTicks = 0;
        elapsedSeconds = 0.0f;
        unityTicks = 0;
        snapshot.failure = LearnFailure::None;
        snapshot.progress = 0.0f;
        snapshot.validSeconds = 0.0f;
    }

    void complete() noexcept
    {
        const std::uint32_t middle = (validTicks + 1) / 2;
        std::uint32_t cumulative = 0;
        std::size_t medianBin = 0;
        for (; medianBin < histogram.size(); ++medianBin)
        {
            cumulative += histogram[medianBin];
            if (cumulative >= middle)
                break;
        }
        snapshot.referenceDb = minimumDb + static_cast<float>(medianBin) * binWidth;
        snapshot.hasValidReference = true;
        snapshot.progress = 1.0f;
        snapshot.validSeconds = requiredValidSeconds;
        snapshot.state = LearnState::Completed;
    }

    std::array<std::uint32_t, binCount> histogram{};
    std::uint32_t validTicks = 0, unityTicks = 0;
    float period = 0.01f, elapsedSeconds = 0.0f;
    LearnSnapshot snapshot;
};
}
