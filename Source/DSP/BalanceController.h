#pragma once

#include "LoudnessDetector.h"

namespace levelsync
{
struct BalanceSettings
{
    float offsetDb = 3.0f;
    float deadbandDb = 0.5f;
    float maxBoostDb = 6.0f;
    float maxCutDb = 6.0f;
    float strength = 1.0f;
    bool enabled = true;
};

class BalanceController
{
public:
    static float targetGainDb(LoudnessMeasurement voice, LoudnessMeasurement music,
                              bool reliableVoiceActivity, const BalanceSettings& settings) noexcept
    {
        if (!settings.enabled || !reliableVoiceActivity || !voice.reliable || !music.reliable
            || !std::isfinite(voice.db) || !std::isfinite(music.db)
            || !std::isfinite(settings.offsetDb) || !std::isfinite(settings.deadbandDb)
            || !std::isfinite(settings.maxBoostDb) || !std::isfinite(settings.maxCutDb)
            || !std::isfinite(settings.strength))
            return 0.0f;

        const float error = music.db + std::clamp(settings.offsetDb, -12.0f, 12.0f) - voice.db;
        const float width = std::clamp(settings.deadbandDb, 0.0f, 2.0f);
        // Continuous dead zone: no step in the target when crossing its edge.
        const float correction = std::copysign(std::max(0.0f, std::abs(error) - width), error);
        return std::clamp(settings.strength, 0.0f, 1.0f)
               * std::clamp(correction, -std::clamp(settings.maxCutDb, 0.0f, 12.0f),
                                        std::clamp(settings.maxBoostDb, 0.0f, 12.0f));
    }
};
}
