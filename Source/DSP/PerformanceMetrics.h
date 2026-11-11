#pragma once

#include "TelemetryQueue.h"
#include <algorithm>
#include <cmath>
#include <limits>

namespace levelsync
{
struct PerformanceMetrics
{
    double meanAbsoluteErrorDb = 0.0;
    float maximumGainDb = -std::numeric_limits<float>::infinity();
    float minimumGainDb = std::numeric_limits<float>::infinity();
    double gainMovementDb = 0.0;
    double peakGainSpeedDbPerSecond = 0.0;
    double convergenceSeconds = -1.0;
    std::size_t validFrames = 0;
};

class PerformanceMetricsAccumulator
{
public:
    explicit PerformanceMetricsAccumulator(double framePeriodSeconds = 0.04) noexcept
        : period(std::max(0.001, framePeriodSeconds)) {}

    void reset() noexcept { *this = PerformanceMetricsAccumulator(period); }

    void push(const TelemetryFrame& frame, bool stableRegion, float convergenceToleranceDb = 0.5f) noexcept
    {
        if (!frame.voiceValid || !frame.musicValid || !frame.vocalActive
            || !std::isfinite(frame.balanceErrorDb) || !std::isfinite(frame.appliedGainDb))
            return;

        if (hasPrevious)
        {
            const double movement = std::abs(frame.appliedGainDb - previousGain);
            result.gainMovementDb += movement;
            result.peakGainSpeedDbPerSecond = std::max(result.peakGainSpeedDbPerSecond, movement / period);
        }
        previousGain = frame.appliedGainDb;
        hasPrevious = true;
        result.maximumGainDb = std::max(result.maximumGainDb, frame.appliedGainDb);
        result.minimumGainDb = std::min(result.minimumGainDb, frame.appliedGainDb);

        if (stableRegion)
        {
            absoluteErrorSum += std::abs(frame.balanceErrorDb);
            ++result.validFrames;
            result.meanAbsoluteErrorDb = absoluteErrorSum / static_cast<double>(result.validFrames);
        }

        elapsed += period;
        const bool insideTolerance = std::abs(frame.balanceErrorDb) <= convergenceToleranceDb;
        if (!insideTolerance)
        {
            sawExcursion = true;
            consecutiveInside = 0;
        }
        else if (sawExcursion && result.convergenceSeconds < 0.0)
        {
            if (++consecutiveInside >= 5)
                result.convergenceSeconds = elapsed;
        }
    }

    PerformanceMetrics get() const noexcept { return result; }

private:
    PerformanceMetrics result;
    double period = 0.04, elapsed = 0.0, absoluteErrorSum = 0.0;
    float previousGain = 0.0f;
    bool hasPrevious = false;
    bool sawExcursion = false;
    int consecutiveInside = 0;
};
}
