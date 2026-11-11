#include "DSP/BalanceEngine.h"
#include "DSP/TelemetryQueue.h"
#include "TestSupport.h"
#include <atomic>
#include <cstdlib>
#include <limits>
#include <new>
#include <vector>

namespace
{
std::atomic<std::size_t> allocationCount{0};
constexpr double pi = 3.14159265358979323846;
float tone(int sample, double rate, float amplitude)
{
    return amplitude * static_cast<float>(std::sin(2.0 * pi * 997.0 * sample / rate));
}
}

// Count allocations to exercise the prepared engine's entire realtime path.
void* operator new(std::size_t size)
{
    allocationCount.fetch_add(1, std::memory_order_relaxed);
    if (auto* p = std::malloc(size == 0 ? 1 : size))
        return p;
    throw std::bad_alloc{};
}
void* operator new[](std::size_t size) { return ::operator new(size); }
void operator delete(void* p) noexcept { std::free(p); }
void operator delete[](void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }
void operator delete[](void* p, std::size_t) noexcept { std::free(p); }

int main()
{
    using namespace levelsync;
    int failures = 0;
    run("correction, continuous deadband, limits and strength", []
    {
        BalanceSettings s;
        s.deadbandDb = 0.0f;
        const LoudnessMeasurement v{-24.0f, true}, m{-26.0f, true};
        near(BalanceController::targetGainDb(v, m, true, s), 1.0, 1.0e-6, "music + offset - voice");
        s.deadbandDb = 0.5f;
        near(BalanceController::targetGainDb(v, {-27.2f, true}, true, s), 0.0, 1.0e-6, "inside deadband");
        near(BalanceController::targetGainDb(v, {-26.49f, true}, true, s), 0.01, 1.0e-5, "continuous boundary");
        near(BalanceController::targetGainDb(v, {0.0f, true}, true, s), 6.0, 1.0e-6, "boost limit");
        near(BalanceController::targetGainDb(v, {-60.0f, true}, true, s), -6.0, 1.0e-6, "cut limit");
        s.strength = 0.0f;
        near(BalanceController::targetGainDb(v, m, true, s), 0.0, 1.0e-6, "zero strength");
        s.strength = 0.5f;
        near(BalanceController::targetGainDb(v, {0.0f, true}, true, s), 3.0, 1.0e-6, "half strength after limit");
    }, failures);

    run("invalid measurements, disabled and silent voice are neutral", []
    {
        BalanceSettings s;
        const LoudnessMeasurement v{-24.0f, true}, m{-20.0f, true};
        near(BalanceController::targetGainDb(v, m, false, s), 0.0, 0.0, "no activity");
        near(BalanceController::targetGainDb({-120.0f, false}, m, true, s), 0.0, 0.0, "silent voice");
        near(BalanceController::targetGainDb(v, {-120.0f, false}, true, s), 0.0, 0.0, "invalid music");
        near(BalanceController::targetGainDb({std::numeric_limits<float>::quiet_NaN(), true}, m, true, s),
             0.0, 0.0, "NaN measurement");
        s.strength = std::numeric_limits<float>::infinity();
        near(BalanceController::targetGainDb(v, m, true, s), 0.0, 0.0, "invalid setting");
        s = {};
        s.enabled = false;
        near(BalanceController::targetGainDb(v, m, true, s), 0.0, 0.0, "disabled");
    }, failures);

    run("attack and release have documented directions and time constants", []
    {
        GainSmoother smoother;
        smoother.prepare(48000.0);
        smoother.setTarget(-6.0f);
        float previous = 1.0f;
        for (int i = 0; i < 12000; ++i)
        {
            const float next = smoother.nextGain();
            require(next <= previous && std::abs(next - previous) < 0.0001f, "smooth monotonic attack");
            previous = next;
        }
        near(smoother.gainDb(), -6.0 * (1.0 - std::exp(-1.0)), 1.0e-4, "attack at 250 ms");
        const float initial = smoother.gainDb();
        smoother.setTarget(0.0f);
        for (int i = 0; i < 33600; ++i)
            smoother.nextGain();
        near(smoother.gainDb(), initial * std::exp(-1.0), 1.0e-4, "release at 700 ms");
    }, failures);

    run("K-weighted calibration, stereo power sum and sample rates", []
    {
        std::array<float, 3> results{};
        std::size_t rateIndex = 0;
        for (const double rate : {44100.0, 48000.0, 96000.0})
        {
            LoudnessDetector mono, stereo;
            mono.prepare(rate);
            stereo.prepare(rate);
            require(!mono.measurement().reliable, "warm-up must be invalid");
            for (int i = 0; i < static_cast<int>(rate * 2.0); ++i)
            {
                const float x = tone(i, rate, 0.1f);
                mono.push({x, 0.0f}, 1);
                stereo.push({x, -x}, 2);
            }
            const auto m = mono.measurement();
            require(m.reliable, "steady tone must be reliable");
            near(m.db, -23.01, 0.06, "997 Hz mono estimate");
            near(stereo.measurement().db - m.db, 3.0103, 0.005, "stereo sum, no phase cancellation");
            results[rateIndex++] = m.db;
        }
        near(results[0], results[1], 0.04, "44.1 vs 48 kHz");
        near(results[2], results[1], 0.04, "96 vs 48 kHz");
    }, failures);

    run("K-weighting rejects DC and weights high frequencies", []
    {
        LoudnessDetector dc, high, low;
        dc.prepare(48000.0); high.prepare(48000.0); low.prepare(48000.0);
        for (int i = 0; i < 144000; ++i)
        {
            dc.push({0.1f, 0.0f}, 1);
            high.push({0.1f * static_cast<float>(std::sin(2.0 * pi * 10000.0 * i / 48000.0)), 0.0f}, 1);
            low.push({0.1f * static_cast<float>(std::sin(2.0 * pi * 100.0 * i / 48000.0)), 0.0f}, 1);
        }
        require(dc.measurement().db < -85.0f, "DC rejection");
        require(high.measurement().db - low.measurement().db > 4.0f, "perceptual weighting");
    }, failures);

    run("activity threshold, minimum duration, hysteresis and hold", []
    {
        VocalActivityDetector detector;
        detector.prepare(48000.0);
        for (int i = 0; i < 1000; ++i) detector.push({0.1f, 0.0f}, 1);
        require(!detector.isActive(), "short event must not qualify");
        for (int i = 0; i < 5000; ++i) detector.push({0.1f, 0.0f}, 1);
        require(detector.isReliable(), "sustained activity");
        for (int i = 0; i < 4800; ++i) detector.push({0.0f, 0.0f}, 1);
        require(detector.isActive() && !detector.isReliable(), "hold indication without chasing silence");
        for (int i = 0; i < 15000; ++i) detector.push({0.0f, 0.0f}, 1);
        require(!detector.isActive(), "hold expires");
        VocalActivityDetector::Settings s;
        s.thresholdDb = -20.0f;
        detector.setSettings(s);
        for (int i = 0; i < 12000; ++i) detector.push({0.01f, 0.0f}, 1);
        require(!detector.isActive(), "configurable threshold");
    }, failures);

    run("synthetic convergence, block independence and sustained instrumental step", []
    {
        for (const double rate : {44100.0, 48000.0, 96000.0})
        {
            float reference = 0.0f;
            for (const int block : {1, 17, 64, 511, 2048})
            {
                BalanceEngine engine;
                engine.prepare(rate, 1);
                EngineSettings s;
                s.balance.deadbandDb = 0.0f;
                engine.setSettings(s);
                const int length = static_cast<int>(rate * 10.0);
                float firstGain = 0.0f;
                for (int start = 0; start < length; start += block)
                {
                    engine.setSettings(s); // Mimics host parameter snapshots per block.
                    for (int i = start; i < std::min(start + block, length); ++i)
                    {
                        const float amplitude = i < length / 2 ? 0.05f : 0.025f;
                        const float music = tone(i, rate, amplitude);
                        std::array<float, 2> v{tone(i, rate, 0.1f), 0.0f};
                        engine.processSample(v, {music, music}, true);
                        if (i == length / 2 - 1) firstGain = engine.snapshot().gainDb;
                        require(std::isfinite(v[0]), "finite synthetic output");
                    }
                }
                near(firstGain, -0.0103, 0.025, "initial relation");
                near(engine.snapshot().gainDb, -6.0, 0.03, "sustained -6 dB instrumental step");
                if (block == 1) reference = engine.snapshot().gainDb;
                else near(engine.snapshot().gainDb, reference, 1.0e-6, "block independent result");
            }
        }
    }, failures);

    run("positive gain convergence and finite pathological input recovery", []
    {
        BalanceEngine engine;
        engine.prepare(48000.0, 2);
        EngineSettings s;
        s.balance.deadbandDb = 0.0f;
        engine.setSettings(s);
        for (int i = 0; i < 288000; ++i)
        {
            const float x = tone(i, 48000.0, 0.05f);
            std::array<float, 2> v{x, -x};
            engine.processSample(v, {x, x}, true);
        }
        near(engine.snapshot().gainDb, 3.0, 0.01, "positive offset converges");
        std::array<float, 2> bad{std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::infinity()};
        engine.processSample(bad, bad, true);
        require(std::isfinite(bad[0]) && std::isfinite(bad[1]), "NaN/Inf sanitised");
        std::array<float, 2> enormous{std::numeric_limits<float>::max(), -std::numeric_limits<float>::max()};
        engine.processSample(enormous, enormous, true);
        require(std::isfinite(enormous[0]) && std::isfinite(enormous[1]), "no overflow in output");
        for (int i = 0; i < 144000; ++i)
        {
            const float x = tone(i, 48000.0, 0.05f);
            std::array<float, 2> v{x, x};
            engine.processSample(v, {x, x}, true);
        }
        require(engine.snapshot().sidechainValid, "detector recovers from invalid samples");
        near(engine.snapshot().gainDb, 3.0, 0.1, "controller recovers");
    }, failures);

    run("silence, background noise, missing and disconnected sidechain", []
    {
        BalanceEngine engine;
        engine.prepare(48000.0, 1);
        for (int i = 0; i < 96000; ++i)
        {
            std::array<float, 2> v{0.00001f, 0.0f};
            const float m = tone(i, 48000.0, 0.2f);
            engine.processSample(v, {m, m}, true);
            near(engine.snapshot().gainDb, 0.0, 0.0, "no noise boost");
        }
        for (int i = 0; i < 288000; ++i)
        {
            const float x = tone(i, 48000.0, 0.05f);
            std::array<float, 2> v{x, 0.0f};
            engine.processSample(v, {x, x}, true);
        }
        near(engine.snapshot().gainDb, 5.5103, 0.025, "establish boost before disconnect");
        for (int i = 0; i < 288000; ++i)
        {
            std::array<float, 2> v{tone(i, 48000.0, 0.05f), 0.0f};
            engine.processSample(v, {}, false);
        }
        near(engine.snapshot().gainDb, 0.0, 0.001, "disconnect returns to unity");
        require(!engine.snapshot().sidechainValid, "missing sidechain flagged");
        for (int i = 0; i < 96000; ++i)
        {
            std::array<float, 2> v{tone(i, 48000.0, 0.05f), 0.0f};
            engine.processSample(v, {}, true); // Enabled bus with no routed signal.
        }
        near(engine.snapshot().gainDb, 0.0, 0.001, "silent connected bus stays neutral");
    }, failures);

    run("voice returns at unity after a prolonged musical stop", []
    {
        BalanceEngine engine;
        engine.prepare(48000.0, 1);
        EngineSettings settings;
        settings.balance.deadbandDb = 0.0f;
        engine.setSettings(settings);

        for (int i = 0; i < 240000; ++i)
        {
            std::array<float, 2> voiceSample{tone(i, 48000.0, 0.1f), 0.0f};
            const float musicSample = tone(i, 48000.0, 0.01f);
            engine.processSample(voiceSample, {musicSample, musicSample}, true);
        }
        require(engine.snapshot().gainDb < -5.8f, "negative correction established before stop");

        for (int i = 0; i < 96000; ++i)
        {
            std::array<float, 2> silence{};
            engine.processSample(silence, {}, true);
        }

        std::array<float, 2> voiceEntry{0.1f, 0.0f};
        engine.processSample(voiceEntry, {}, true);
        near(engine.snapshot().gainDb, 0.0, 0.1, "prolonged music silence returns vocal gain to unity");
        near(voiceEntry[0], 0.1, 0.002, "voice enters at its original level");
    }, failures);

    run("music silence detector applies hold, dBFS hysteresis and release", []
    {
        for (const double rate : {44100.0, 48000.0, 96000.0})
        {
            MusicSilenceDetector detector;
            detector.prepare(rate);
            const auto pushFor = [&](float seconds, float amplitude)
            {
                const int count = static_cast<int>(std::lround(rate * seconds));
                for (int i = 0; i < count; ++i)
                {
                    const float x = amplitude * static_cast<float>(std::sin(2.0 * pi * 997.0 * i / rate));
                    detector.push({x, x}, true);
                }
            };

            pushFor(1.0f, 0.1f);
            require(detector.isMusicActive(), "normal music is active");
            require(detector.getLevelDbfs() > -25.0f && detector.getLevelDbfs() < -15.0f,
                    "detector reports unweighted RMS energy in dBFS");
            pushFor(0.19f, 0.0f);
            require(detector.isMusicActive(), "silence hold preserves last valid state");
            pushFor(0.07f, 0.0f);
            require(!detector.isMusicActive() && detector.isProtectionActive(true),
                    "sustained silence activates protection");
            pushFor(0.39f, 0.004f);
            require(!detector.isMusicActive(), "music return must satisfy release time");
            pushFor(0.03f, 0.004f);
            require(detector.isMusicActive(), "confirmed music releases protection");

            for (int i = 0; i < static_cast<int>(rate); ++i)
            {
                const float amplitude = (i / 100) % 2 == 0 ? 0.0015f : 0.0030f;
                detector.push({amplitude, amplitude}, true);
            }
            require(detector.isMusicActive(), "hysteresis prevents rapid state chatter near threshold");
            detector.push({}, false);
            require(!detector.isProtectionActive(false), "disconnected sidechain is not labelled protected silence");
        }
    }, failures);

    run("music silence protection is voice-independent, smooth and finite", []
    {
        for (const int blockSize : {1, 17, 64, 511, 2048})
        {
            BalanceEngine engine;
            engine.prepare(48000.0, 1);
            EngineSettings settings;
            settings.balance.deadbandDb = 0.0f;
            engine.setSettings(settings);
            for (int start = 0; start < 240000; start += blockSize)
                for (int i = start; i < std::min(start + blockSize, 240000); ++i)
                {
                    std::array<float, 2> v{tone(i, 48000.0, 0.1f), 0.0f};
                    const float m = tone(i, 48000.0, 0.01f);
                    engine.processSample(v, {m, m}, true);
                }
            require(engine.snapshot().gainDb < -5.8f, "pre-stop attenuation established");

            float previousGain = engine.snapshot().gainDb;
            for (int i = 0; i < 24000; ++i)
            {
                std::array<float, 2> silentVoice{};
                engine.processSample(silentVoice, {}, true);
                const float gain = engine.snapshot().gainDb;
                require(std::isfinite(gain) && std::abs(gain - previousGain) < 0.004f,
                        "silence transition has no gain click or invalid value");
                previousGain = gain;
            }
            require(engine.snapshot().musicSilenceProtectionActive, "protection active after 500 ms stop");
            near(engine.snapshot().gainDb, 0.0, 0.12, "gain is approximately unity after 500 ms stop");

            std::array<float, 2> entryAfter500Ms{0.1f, 0.0f};
            engine.processSample(entryAfter500Ms, {}, true);
            near(entryAfter500Ms[0], 0.1, 0.002,
                 "voice entering after 500 ms of music silence keeps its original level");

            for (int i = 0; i < 48000; ++i)
            {
                std::array<float, 2> voiceOnly{tone(i, 48000.0, 0.1f), 0.0f};
                engine.processSample(voiceOnly, {}, true);
                require(std::isfinite(voiceOnly[0]), "voice-only output remains finite");
            }
            near(engine.snapshot().gainDb, 0.0, 0.01, "voice remains at unity during musical silence");

            for (int i = 0; i < 96000; ++i)
            {
                const float x = tone(i, 48000.0, 0.1f);
                std::array<float, 2> v{x, 0.0f};
                engine.processSample(v, {x, x}, true);
            }
            require(engine.snapshot().musicActive && !engine.snapshot().musicSilenceProtectionActive,
                    "music return reactivates normal controller");
        }
    }, failures);

    run("Learn Balance pauses while music silence protection is active", []
    {
        BalanceEngine engine;
        engine.prepare(48000.0, 1);
        engine.startLearning();
        for (int i = 0; i < 72000; ++i)
        {
            const float x = tone(i, 48000.0, 0.1f);
            std::array<float, 2> v{x, 0.0f};
            engine.processSample(v, {x, x}, true);
        }
        const float beforeSilence = engine.snapshot().learning.validSeconds;
        for (int i = 0; i < 96000; ++i)
        {
            std::array<float, 2> v{tone(i, 48000.0, 0.1f), 0.0f};
            engine.processSample(v, {}, true);
        }
        near(engine.snapshot().learning.validSeconds, beforeSilence, 0.051,
             "silent instrumental adds no calibration time");
        require(engine.snapshot().learning.state == LearnState::Learning,
                "learning remains paused and resumable");
    }, failures);

    run("phrase end and background noise cannot keep requesting boost", []
    {
        BalanceEngine engine;
        engine.prepare(48000.0, 1);
        for (int i = 0; i < 240000; ++i)
        {
            const float x = tone(i, 48000.0, 0.05f);
            std::array<float, 2> v{x, 0.0f};
            engine.processSample(v, {x, x}, true);
        }
        require(engine.snapshot().gainDb > 5.4f, "voice initially boosted");
        for (int i = 0; i < 576000; ++i)
        {
            const float m = tone(i, 48000.0, 0.05f);
            std::array<float, 2> v{0.00001f, 0.0f};
            engine.processSample(v, {m, m}, true);
        }
        near(engine.snapshot().gainDb, 0.0, 0.003, "gain returns to unity after phrase");
        require(!engine.snapshot().vocalActive, "vocal activity expires");
    }, failures);

    run("no allocation in prepared DSP processing", []
    {
        BalanceEngine engine;
        engine.prepare(96000.0, 2);
        EngineSettings s;
        const auto before = allocationCount.load();
        for (int i = 0; i < 96000; ++i)
        {
            if (i % 17 == 0) engine.setSettings(s);
            const float x = tone(i, 96000.0, 0.1f);
            std::array<float, 2> v{x, x};
            engine.processSample(v, {x, x}, true);
        }
        const auto after = allocationCount.load();
        require(before == after, "audio path allocated memory");
    }, failures);

    run("ERB analysis distinguishes spectral overlap at supported sample rates", []
    {
        for (const double rate : {44100.0, 48000.0, 96000.0})
        {
            const auto analyse = [rate](double voiceHz, double musicHz)
            {
                SpectralAnalyzer voice, music;
                voice.prepare(rate, 1);
                music.prepare(rate, 2);
                for (int i = 0; i < static_cast<int>(rate); ++i)
                {
                    const float v = 0.1f * static_cast<float>(std::sin(2.0 * pi * voiceHz * i / rate));
                    const float m = 0.1f * static_cast<float>(std::sin(2.0 * pi * musicHz * i / rate));
                    voice.push({v, 0.0f});
                    music.push({m, m});
                }
                return MaskingEstimator::estimate(voice.getSnapshot(), music.getSnapshot());
            };
            const auto overlap = analyse(1000.0, 1000.0);
            const auto separated = analyse(1000.0, 100.0);
            require(overlap.valid && separated.valid, "steady spectra produce estimates");
            require(overlap.index > separated.index + 0.25f, "same-band competition exceeds separated bands");
            require(std::isfinite(overlap.index) && overlap.index <= 1.0f, "masking index finite and bounded");
        }

        SpectralAnalyzer silentVoice, music;
        silentVoice.prepare(48000.0, 1); music.prepare(48000.0, 2);
        for (int i = 0; i < 48000; ++i)
        {
            const float m = tone(i, 48000.0, 0.1f);
            silentVoice.push({0.0f, 0.0f}); music.push({m, m});
        }
        require(!MaskingEstimator::estimate(silentVoice.getSnapshot(), music.getSnapshot()).valid,
                "silent voice cannot claim masking");
    }, failures);

    run("spectral learner uses medians and preserves reference on insufficient data", []
    {
        SpectralReferenceLearner learner;
        SpectralReference reference;
        reference.valid = true;
        reference.index = 0.17f;
        reference.competition.fill(0.17f);
        learner.start();
        MaskingEstimate estimate;
        estimate.valid = true;
        estimate.confidence = 1.0f;
        estimate.relevance.fill(1.0f);
        for (int frame = 0; frame < 101; ++frame)
        {
            estimate.index = frame < 70 ? 0.4f : 0.95f;
            estimate.competition.fill(estimate.index);
            learner.push(estimate);
        }
        require(learner.complete(reference), "enough frames complete spectral learning");
        near(reference.index, 0.4, 0.011, "spectral median rejects outliers");

        SpectralReferenceLearner shortSession;
        shortSession.start();
        for (int i = 0; i < 19; ++i) shortSession.push(estimate);
        const auto previous = reference;
        require(!shortSession.complete(reference), "short session rejected");
        near(reference.index, previous.index, 0.0, "failed learning preserves prior profile");
    }, failures);

    run("perceptual correction is optional, smoothed and globally bounded", []
    {
        MaskingEstimate current;
        current.valid = true;
        current.confidence = 1.0f;
        current.index = 0.8f;
        current.competition.fill(0.8f);
        current.relevance.fill(1.0f);
        SpectralReference reference;
        reference.valid = true;
        reference.index = 0.2f;
        reference.competition.fill(0.2f);
        PerceptualBalanceController controller;
        controller.prepare(0.01f);
        PerceptualSettings settings;
        near(controller.process(0.0f, 6.0f, current, reference, true, settings), 0.0, 0.0,
             "Off mode is exactly neutral");
        settings.mode = PerceptualMode::balanced;
        const float first = controller.process(0.0f, 6.0f, current, reference, true, settings);
        require(first > 0.0f && first < 0.1f, "transient is response-smoothed");
        float settled = first;
        for (int i = 0; i < 1000; ++i)
            settled = controller.process(0.0f, 6.0f, current, reference, true, settings);
        require(settled > first && settled <= settings.maxBoostDb, "sustained overlap raises bounded correction");
        near(controller.process(6.0f, 6.0f, current, reference, true, settings), 0.0, 1.0e-6,
             "global boost limit prevents double counting");
        reference.valid = false;
        for (int i = 0; i < 1000; ++i)
            settled = controller.process(0.0f, 6.0f, current, reference, true, settings);
        near(settled, 0.0, 1.0e-5, "missing reference decays to neutral");
        require(std::isfinite(settled), "perceptual output stays finite");
    }, failures);

    run("reference learner constant levels and preparing unity gate", []
    {
        ReferenceLearner learner;
        learner.prepare(0.01f);
        learner.setStoredReference(2.0f, true);
        learner.start();
        const LoudnessMeasurement voice{-20.0f, true}, music{-23.2f, true};
        for (int i = 0; i < 100; ++i)
            learner.process(0.2f, voice, music, true, true);
        require(learner.getSnapshot().state == LearnState::Preparing, "gain must reach unity first");
        for (int i = 0; i < 10; ++i)
            learner.process(0.0f, voice, music, true, true);
        require(learner.getSnapshot().state == LearnState::Learning, "stable unity starts collection");
        require(learner.getSnapshot().validSeconds == 0.0f, "preparing samples excluded");
        for (int i = 0; i < 500; ++i)
            learner.process(0.0f, voice, music, true, true);
        const auto result = learner.getSnapshot();
        require(result.state == LearnState::Completed && result.hasValidReference, "learning completes");
        near(result.referenceDb, 3.2, 0.051, "constant relation learned");
        near(result.progress, 1.0, 0.0, "full progress");
    }, failures);

    run("reference learner robust median rejects variable outliers", []
    {
        ReferenceLearner learner;
        learner.prepare(0.01f);
        learner.start();
        for (int i = 0; i < 10; ++i)
            learner.process(0.0f, {-20.0f, true}, {-23.0f, true}, true, true);
        for (int i = 0; i < 500; ++i)
        {
            const float difference = i < 350 ? 3.0f : (i < 425 ? -8.0f : 11.0f);
            learner.process(0.0f, {-20.0f, true}, {-20.0f - difference, true}, true, true);
        }
        near(learner.getSnapshot().referenceDb, 3.0, 0.051, "time-weighted median");
    }, failures);

    run("reference learner pauses invalid voice and sidechain", []
    {
        ReferenceLearner learner;
        learner.prepare(0.01f);
        learner.start();
        for (int i = 0; i < 10; ++i)
            learner.process(0.0f, {-20.0f, true}, {-23.0f, true}, true, true);
        for (int i = 0; i < 100; ++i)
            learner.process(0.0f, {-20.0f, false}, {-23.0f, true}, true, true);
        for (int i = 0; i < 100; ++i)
            learner.process(0.0f, {-20.0f, true}, {-23.0f, true}, false, true);
        for (int i = 0; i < 100; ++i)
            learner.process(0.0f, {-20.0f, true}, {-23.0f, true}, true, false);
        near(learner.getSnapshot().validSeconds, 0.0, 0.0, "invalid periods add no progress");
        learner.process(0.0f, {-20.0f, true}, {-23.0f, true}, true, true);
        near(learner.getSnapshot().validSeconds, 0.01, 1.0e-6, "valid period resumes progress");
    }, failures);

    run("reference learner cancellation and timeout preserve prior reference", []
    {
        ReferenceLearner learner;
        learner.prepare(0.01f);
        learner.setStoredReference(4.2f, true);
        learner.start();
        for (int i = 0; i < 50; ++i)
            learner.process(0.0f, {-20.0f, true}, {-25.0f, true}, true, true);
        learner.cancel();
        auto result = learner.getSnapshot();
        require(result.state == LearnState::Cancelled, "cancelled state");
        near(result.referenceDb, 4.2, 1.0e-6, "cancel preserves reference");
        learner.start();
        for (int i = 0; i < 3001; ++i)
            learner.process(0.0f, {-120.0f, false}, {-120.0f, false}, false, false);
        result = learner.getSnapshot();
        require(result.state == LearnState::Failed && result.failure == LearnFailure::TimedOut,
                "invalid session times out");
        near(result.referenceDb, 4.2, 1.0e-6, "failure preserves reference");
    }, failures);

    run("learning is sample-clock deterministic and uses pre-gain voice", []
    {
        for (const int hostBlock : {1, 17, 64, 511, 2048})
        {
            BalanceEngine engine;
            engine.prepare(48000.0, 1);
            EngineSettings settings;
            settings.balance.offsetDb = -6.0f;
            settings.balance.deadbandDb = 0.0f;
            engine.setSettings(settings);
            for (int i = 0; i < 240000; ++i)
            {
                std::array<float, 2> v{tone(i, 48000.0, 0.1f), 0.0f};
                const float m = tone(i, 48000.0, 0.1f);
                engine.processSample(v, {m, m}, true);
            }
            require(engine.snapshot().gainDb < -2.9f, "automatic gain established before learning");
            engine.startLearning();
            float previous = engine.snapshot().gainDb;
            for (int start = 0; start < 600000; start += hostBlock)
                for (int i = start; i < std::min(start + hostBlock, 600000); ++i)
                {
                    std::array<float, 2> v{tone(i, 48000.0, 0.1f), 0.0f};
                    const float m = tone(i, 48000.0, 0.1f);
                    engine.processSample(v, {m, m}, true);
                    require(std::abs(engine.snapshot().gainDb - previous) < 0.001f,
                            "no gain discontinuity at state transition");
                    previous = engine.snapshot().gainDb;
                    if (engine.snapshot().learning.state == LearnState::Completed)
                        break;
                }
            const auto result = engine.snapshot();
            require(result.learning.state == LearnState::Completed, "engine learning completed");
            near(result.learning.referenceDb, -3.0103, 0.06, "pre-gain mono voice vs stereo music");
        }
    }, failures);

    run("silence return time is independent from attack and release", []
    {
        GainSmoother fast, slow;
        fast.prepare(48000.0); slow.prepare(48000.0);
        fast.setSilenceReturnTime(100.0f);
        slow.setSilenceReturnTime(2000.0f);
        fast.setTarget(6.0f); slow.setTarget(6.0f);
        for (int i = 0; i < 240000; ++i) { fast.nextGain(); slow.nextGain(); }
        fast.setNeutralTarget(); slow.setNeutralTarget();
        for (int i = 0; i < 4800; ++i) { fast.nextGain(); slow.nextGain(); }
        near(fast.gainDb(), 6.0 * std::exp(-1.0), 0.002, "100 ms neutral return");
        require(slow.gainDb() > 5.6f, "slow neutral return remains gradual");
    }, failures);

    run("fixed SPSC telemetry queue preserves order and never allocates", []
    {
        TelemetryQueue<4> queue;
        const auto before = allocationCount.load();
        std::array<bool, 3> pushed{};
        for (int i = 0; i < 3; ++i)
        {
            TelemetryFrame frame;
            frame.appliedGainDb = static_cast<float>(i);
            pushed[static_cast<std::size_t>(i)] = queue.push(frame);
        }
        const bool fullResult = queue.push({});
        std::array<bool, 3> popped{};
        std::array<float, 3> values{};
        for (int i = 0; i < 3; ++i)
        {
            TelemetryFrame frame;
            popped[static_cast<std::size_t>(i)] = queue.pop(frame);
            values[static_cast<std::size_t>(i)] = frame.appliedGainDb;
        }
        TelemetryFrame empty;
        const bool emptyResult = queue.pop(empty);
        const auto after = allocationCount.load();
        for (int i = 0; i < 3; ++i)
        {
            require(pushed[static_cast<std::size_t>(i)], "queue accepts available slot");
            require(popped[static_cast<std::size_t>(i)], "queued frame available");
            near(values[static_cast<std::size_t>(i)], i, 0.0, "FIFO order");
        }
        require(!fullResult, "full queue drops frame");
        require(!emptyResult, "empty queue reports false");
        require(before == after, "queue operations allocate nothing");
    }, failures);

    run("long synthetic run survives changing levels, silence and disconnects", []
    {
        for (const double rate : {44100.0, 48000.0, 96000.0})
        {
            BalanceEngine engine;
            engine.prepare(rate, 2);
            EngineSettings settings;
            settings.activity.holdMs = 400.0f;
            settings.silenceReturnMs = 900.0f;
            engine.setSettings(settings);
            const int samples = static_cast<int>(rate * 12.0);
            for (int i = 0; i < samples; ++i)
            {
                const double seconds = i / rate;
                const bool vocalSilence = seconds > 4.0 && seconds < 5.0;
                const bool sidechain = !(seconds > 8.0 && seconds < 9.0);
                const float vocalAmplitude = vocalSilence ? 0.0f : (seconds < 6.0 ? 0.08f : 0.03f);
                const float musicAmplitude = (i / 12000) % 2 == 0 ? 0.025f : 0.12f;
                const float v = tone(i, rate, vocalAmplitude);
                const float m = tone(i, rate, musicAmplitude);
                std::array<float, 2> voice{v, -v};
                engine.processSample(voice, {m, m}, sidechain);
                require(std::isfinite(voice[0]) && std::isfinite(voice[1]), "long-run output finite");
                require(std::abs(engine.snapshot().gainDb) <= 6.001f, "long-run gain bounded");
            }
        }
    }, failures);

    std::cout << "DSP failures: " << failures << '\n';
    return failures == 0 ? 0 : 1;
}
