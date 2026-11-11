#pragma once

#include "MaskingEstimator.h"

namespace levelsync
{
enum class PerceptualMode : int { off, gentle, balanced, strong };

struct PerceptualSettings
{
    PerceptualMode mode = PerceptualMode::off;
    float strength = 0.5f;
    float maxBoostDb = 2.0f;
    float responseMs = 500.0f;
};

class PerceptualBalanceController
{
public:
    void prepare(float controlPeriodSeconds) noexcept
    {
        period = std::clamp(controlPeriodSeconds, 0.001f, 0.1f);
        smoothedExcess = correction = 0.0f;
    }

    float process(float baseGainDb, float globalMaxBoostDb, const MaskingEstimate& masking,
                  const SpectralReference& reference, bool vocalActive,
                  const PerceptualSettings& settings) noexcept
    {
        const bool enabled = settings.mode != PerceptualMode::off && vocalActive
                             && masking.valid && reference.valid;
        const float raw = enabled ? MaskingEstimator::relativeExcess(masking, reference) : 0.0f;
        const float response = std::isfinite(settings.responseMs)
                                   ? std::clamp(settings.responseMs, 50.0f, 3000.0f) : 500.0f;
        const float coefficient = std::exp(-period / (response * 0.001f));
        smoothedExcess = raw + coefficient * (smoothedExcess - raw);
        const float modeScale = settings.mode == PerceptualMode::gentle ? 0.5f
                              : settings.mode == PerceptualMode::balanced ? 1.0f
                              : settings.mode == PerceptualMode::strong ? 1.5f : 0.0f;
        const float loudnessHeadroom = globalMaxBoostDb > 0.0f
            ? 1.0f - std::clamp(std::max(baseGainDb, 0.0f) / globalMaxBoostDb, 0.0f, 1.0f) : 0.0f;
        correction = smoothedExcess * std::clamp(settings.strength, 0.0f, 1.0f)
                     * std::clamp(settings.maxBoostDb, 0.0f, 6.0f) * modeScale
                     * masking.confidence * loudnessHeadroom;
        correction = std::clamp(correction, 0.0f, std::clamp(settings.maxBoostDb, 0.0f, 6.0f));
        return correction;
    }

    float getSmoothedIndex() const noexcept { return smoothedExcess; }
    float getCorrectionDb() const noexcept { return correction; }

private:
    float period = 0.01f, smoothedExcess = 0.0f, correction = 0.0f;
};
}
