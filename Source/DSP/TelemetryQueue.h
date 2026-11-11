#pragma once

#include "SpectralAnalyzer.h"

#include <array>
#include <atomic>
#include <cstddef>

namespace levelsync
{
struct TelemetryFrame
{
    float voiceDb = -120.0f;
    float musicDb = -120.0f;
    float outputVoiceDb = -120.0f;
    float currentBalanceDb = 0.0f;
    float targetBalanceDb = 0.0f;
    float balanceErrorDb = 0.0f;
    float targetGainDb = 0.0f;
    float appliedGainDb = 0.0f;
    bool voiceValid = false;
    bool musicValid = false;
    bool vocalActive = false;
    bool boostLimited = false;
    bool cutLimited = false;
    float maskingIndex = 0.0f;
    float maskingConfidence = 0.0f;
    float perceptualCorrectionDb = 0.0f;
    bool spectralReferenceValid = false;
    bool musicActive = false;
    bool musicSilenceProtectionActive = false;
    float musicLevelDbfs = -120.0f;
    std::array<float, spectralBandCount> voiceBandsDb{};
    std::array<float, spectralBandCount> musicBandsDb{};
    std::array<float, spectralBandCount> competition{};
    std::array<float, spectralBandCount> referenceCompetition{};
};

// Single producer (audio) / single consumer (GUI), fixed storage. New frames
// are dropped when the GUI is absent or too slow; the audio thread never waits.
template <std::size_t Capacity>
class TelemetryQueue
{
public:
    static_assert(Capacity >= 2, "Telemetry queue needs at least two slots");

    bool push(const TelemetryFrame& frame) noexcept
    {
        const auto write = writeIndex.load(std::memory_order_relaxed);
        const auto next = increment(write);
        if (next == readIndex.load(std::memory_order_acquire))
            return false;
        frames[write] = frame;
        writeIndex.store(next, std::memory_order_release);
        return true;
    }

    bool pop(TelemetryFrame& frame) noexcept
    {
        const auto read = readIndex.load(std::memory_order_relaxed);
        if (read == writeIndex.load(std::memory_order_acquire))
            return false;
        frame = frames[read];
        readIndex.store(increment(read), std::memory_order_release);
        return true;
    }

    void discardPending() noexcept
    {
        TelemetryFrame ignored;
        while (pop(ignored)) {}
    }

private:
    static constexpr std::size_t increment(std::size_t value) noexcept
    {
        return (value + 1) % Capacity;
    }

    std::array<TelemetryFrame, Capacity> frames{};
    std::atomic<std::size_t> writeIndex{0};
    std::atomic<std::size_t> readIndex{0};
};
}
