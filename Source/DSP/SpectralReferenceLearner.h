#pragma once

#include "MaskingEstimator.h"
#include <array>
#include <cstdint>

namespace levelsync
{
class SpectralReferenceLearner
{
public:
    void start() noexcept
    {
        for (auto& band : histograms) band.fill(0);
        indexHistogram.fill(0);
        frames = 0;
    }

    void push(const MaskingEstimate& estimate) noexcept
    {
        if (!estimate.valid)
            return;
        for (std::size_t band = 0; band < spectralBandCount; ++band)
            ++histograms[band][toBin(estimate.competition[band])];
        ++indexHistogram[toBin(estimate.index)];
        ++frames;
    }

    bool complete(SpectralReference& destination) const noexcept
    {
        if (frames < 20)
            return false;
        SpectralReference candidate;
        for (std::size_t band = 0; band < spectralBandCount; ++band)
            candidate.competition[band] = median(histograms[band]);
        candidate.index = median(indexHistogram);
        candidate.valid = true;
        destination = candidate;
        return true;
    }

private:
    static constexpr std::size_t bins = 101;
    static std::size_t toBin(float value) noexcept
    {
        return static_cast<std::size_t>(std::clamp(static_cast<int>(std::lround(value * 100.0f)), 0, 100));
    }
    float median(const std::array<std::uint32_t, bins>& histogram) const noexcept
    {
        const auto middle = (frames + 1) / 2;
        std::uint32_t cumulative = 0;
        for (std::size_t i = 0; i < bins; ++i)
            if ((cumulative += histogram[i]) >= middle)
                return static_cast<float>(i) * 0.01f;
        return 0.0f;
    }

    std::array<std::array<std::uint32_t, bins>, spectralBandCount> histograms{};
    std::array<std::uint32_t, bins> indexHistogram{};
    std::uint32_t frames = 0;
};
}
