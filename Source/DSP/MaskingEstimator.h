#pragma once

#include "SpectralAnalyzer.h"

namespace levelsync
{
struct MaskingEstimate
{
    std::array<float, spectralBandCount> competition{};
    std::array<float, spectralBandCount> relevance{};
    float index = 0.0f;
    float confidence = 0.0f;
    bool valid = false;
};

struct SpectralReference
{
    std::array<float, spectralBandCount> competition{};
    float index = 0.0f;
    bool valid = false;
};

class MaskingEstimator
{
public:
    static MaskingEstimate estimate(const SpectralSnapshot& voice,
                                    const SpectralSnapshot& music) noexcept
    {
        MaskingEstimate result;
        if (!voice.valid || !music.valid || voice.totalEnergy <= 1.0e-10f)
            return result;

        const float peakVoice = *std::max_element(voice.energy.begin(), voice.energy.end());
        const float gate = std::max(peakVoice * 0.000316f, 1.0e-12f); // -35 dB from vocal peak.
        double weightedCompetition = 0.0, weightSum = 0.0, coveredVoice = 0.0;
        for (std::size_t band = 0; band < spectralBandCount; ++band)
        {
            const float v = std::max(voice.energy[band], 0.0f);
            const float m = std::max(music.energy[band], 0.0f);
            if (v < gate)
                continue;
            const float hz = voice.centreHz[band];
            const float presence = std::clamp((hz - 120.0f) / 350.0f, 0.0f, 1.0f)
                                   * std::clamp((10000.0f - hz) / 3500.0f, 0.0f, 1.0f);
            const float vocalShare = v / std::max(voice.totalEnergy, 1.0e-12f);
            const float weight = presence * std::sqrt(vocalShare);
            const float ratio = m / (m + v + 1.0e-15f);
            // Smooth compression avoids treating small ratio changes near 1 as proof of masking.
            const float competition = std::sqrt(std::clamp(ratio, 0.0f, 1.0f));
            result.competition[band] = competition;
            result.relevance[band] = weight;
            weightedCompetition += weight * competition;
            weightSum += weight;
            coveredVoice += v;
        }
        result.index = weightSum > 1.0e-6 ? static_cast<float>(weightedCompetition / weightSum) : 0.0f;
        const float coverage = static_cast<float>(coveredVoice / std::max<double>(voice.totalEnergy, 1.0e-12));
        result.confidence = std::clamp(coverage * static_cast<float>(weightSum / (weightSum + 0.35)), 0.0f, 1.0f);
        result.valid = weightSum > 0.02 && result.confidence > 0.05f
                       && std::isfinite(result.index);
        return result;
    }

    static float relativeExcess(const MaskingEstimate& current,
                                const SpectralReference& reference) noexcept
    {
        if (!current.valid || !reference.valid)
            return 0.0f;
        double excess = 0.0, weights = 0.0;
        for (std::size_t band = 0; band < spectralBandCount; ++band)
        {
            const float weight = current.relevance[band];
            excess += weight * std::max(0.0f, current.competition[band] - reference.competition[band]);
            weights += weight;
        }
        return weights > 1.0e-6 ? std::clamp(static_cast<float>(excess / weights), 0.0f, 1.0f) : 0.0f;
    }
};
}
