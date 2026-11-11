#include "DSP/BalanceEngine.h"
#include "DSP/PerformanceMetrics.h"
#include "TestSupport.h"
#include <iostream>

namespace
{
constexpr double pi = 3.14159265358979323846;
float tone(int sample, float amplitude)
{
    return amplitude * static_cast<float>(std::sin(2.0 * pi * 997.0 * sample / 48000.0));
}
}

int main()
{
    using namespace levelsync;
    BalanceEngine engine;
    engine.prepare(48000.0, 1);
    EngineSettings settings;
    settings.balance.offsetDb = 3.0f;
    settings.balance.deadbandDb = 0.0f;
    engine.setSettings(settings);

    PerformanceMetricsAccumulator metrics(0.04);
    constexpr int totalSamples = 20 * 48000;
    constexpr int stepSample = 8 * 48000;
    constexpr int telemetryInterval = 1920;
    for (int sample = 0; sample < totalSamples; ++sample)
    {
        std::array<float, 2> voice{tone(sample, 0.1f), 0.0f};
        const float musicAmplitude = sample < stepSample ? 0.04f : 0.1f;
        const float music = tone(sample, musicAmplitude);
        engine.processSample(voice, {music, music}, true);

        if (sample >= stepSample && sample % telemetryInterval == 0)
        {
            const auto snapshot = engine.snapshot();
            TelemetryFrame frame;
            frame.voiceDb = snapshot.voice.db;
            frame.musicDb = snapshot.music.db;
            frame.outputVoiceDb = snapshot.voice.db + snapshot.gainDb;
            frame.currentBalanceDb = snapshot.voice.db - snapshot.music.db;
            frame.targetBalanceDb = settings.balance.offsetDb;
            frame.balanceErrorDb = frame.targetBalanceDb
                                   - (frame.outputVoiceDb - frame.musicDb);
            frame.appliedGainDb = snapshot.gainDb;
            frame.voiceValid = snapshot.voice.reliable;
            frame.musicValid = snapshot.music.reliable;
            frame.vocalActive = snapshot.vocalActive;
            metrics.push(frame, sample >= stepSample + 6 * 48000);
        }
    }

    const auto result = metrics.get();
    require(result.validFrames > 100, "enough stable valid frames");
    require(std::isfinite(result.meanAbsoluteErrorDb), "finite MAE");
    require(result.maximumGainDb <= 6.001f && result.minimumGainDb >= -6.001f, "gain limits respected");
    require(result.gainMovementDb > 0.0 && std::isfinite(result.peakGainSpeedDbPerSecond),
            "gain movement measured");
    require(result.convergenceSeconds > 0.0, "convergence measured after sustained step");

    std::cout << "Balance Error MAE (stable): " << result.meanAbsoluteErrorDb << " dB\n"
              << "Maximum Gain: " << result.maximumGainDb << " dB\n"
              << "Minimum Gain: " << result.minimumGainDb << " dB\n"
              << "Gain Movement: " << result.gainMovementDb << " dB cumulative\n"
              << "Peak Gain Speed: " << result.peakGainSpeedDbPerSecond << " dB/s\n"
              << "Convergence Time: " << result.convergenceSeconds << " s\n"
              << "Stable Valid Frames: " << result.validFrames << '\n';
    return 0;
}
