#pragma once

#include "BalanceController.h"
#include "GainSmoother.h"
#include "VocalActivityDetector.h"
#include "MusicSilenceDetector.h"
#include "ReferenceLearner.h"
#include "PerceptualBalanceController.h"
#include "SpectralReferenceLearner.h"
#include <limits>

namespace levelsync
{
struct EngineSettings
{
    BalanceSettings balance;
    float attackMs = 250.0f, releaseMs = 700.0f;
    float silenceReturnMs = 1500.0f;
    MusicSilenceDetector::Settings musicSilence;
    float musicSilenceReturnMs = 300.0f;
    PerceptualSettings perceptual;
    VocalActivityDetector::Settings activity;
};

struct MeterSnapshot
{
    LoudnessMeasurement voice, music;
    float gainDb = 0.0f;
    float targetGainDb = 0.0f;
    float balanceErrorDb = 0.0f;
    bool vocalActive = false, sidechainValid = false;
    bool musicActive = false, musicSilenceProtectionActive = false;
    float musicLevelDbfs = -120.0f;
    bool boostLimited = false, cutLimited = false;
    float maskingIndex = 0.0f, maskingConfidence = 0.0f, perceptualCorrectionDb = 0.0f;
    bool spectralReferenceValid = false;
    std::array<float, spectralBandCount> voiceBandsDb{}, musicBandsDb{}, competition{}, referenceCompetition{};
    LearnSnapshot learning;
};

// Pure C++ DSP; no JUCE, host buffers, locks or allocation in the sample path.
class BalanceEngine
{
public:
    void prepare(double sampleRate, int mainChannels)
    {
        const auto rate = std::isfinite(sampleRate) && sampleRate >= 8000.0 ? sampleRate : 48000.0;
        channels = std::clamp(mainChannels, 1, 2);
        voice.prepare(rate);
        music.prepare(rate);
        activity.prepare(rate);
        musicSilence.prepare(rate);
        gain.prepare(rate);
        controlInterval = std::max(1, static_cast<int>(std::lround(rate * 0.01)));
        controlCountdown = 0;
        connectedSamples = 0;
        meters = {};
        learner.prepare(static_cast<float>(controlInterval / rate));
        voiceSpectrum.prepare(rate, channels);
        musicSpectrum.prepare(rate, 2);
        perceptual.prepare(static_cast<float>(controlInterval / rate));
        currentMasking = {};
        spectralSessionCompleted = false;
        setSettings(EngineSettings{});
    }

    void startLearning() noexcept
    {
        learner.start();
        spectralLearner.start();
        spectralSessionCompleted = false;
    }
    void cancelLearning() noexcept { learner.cancel(); }
    void acknowledgeLearning() noexcept { learner.acknowledgeCompletion(); }
    void setStoredReference(float db, bool valid) noexcept { learner.setStoredReference(db, valid); }
    void setSpectralReference(const SpectralReference& reference) noexcept { spectralReference = reference; }
    SpectralReference getSpectralReference() const noexcept { return spectralReference; }

    void setSettings(const EngineSettings& value) noexcept
    {
        settings = value;
        gain.setTimes(settings.attackMs, settings.releaseMs);
        gain.setSilenceReturnTime(settings.silenceReturnMs);
        gain.setMusicSilenceReturnTime(settings.musicSilenceReturnMs);
        activity.setSettings(settings.activity);
        musicSilence.setSettings(settings.musicSilence);
    }

    void processSample(std::array<float, 2>& vocal,
                       const std::array<float, 2>& instrumental, bool sidechainPresent) noexcept
    {
        voice.push(vocal, channels);
        music.push(sidechainPresent ? instrumental : std::array<float, 2>{}, 2);
        musicSilence.push(instrumental, sidechainPresent);
        activity.push(vocal, channels);
        const bool newVoiceSpectrum = voiceSpectrum.push(vocal);
        const bool newMusicSpectrum = musicSpectrum.push(sidechainPresent ? instrumental : std::array<float, 2>{});
        if (newVoiceSpectrum && newMusicSpectrum)
        {
            currentMasking = MaskingEstimator::estimate(voiceSpectrum.getSnapshot(), musicSpectrum.getSnapshot());
            if (learner.getSnapshot().state == LearnState::Learning && activity.isReliable()
                && sidechainPresent
                && (!settings.musicSilence.enabled || musicSilence.hasMusicSignal())
                && currentMasking.valid)
                spectralLearner.push(currentMasking);
        }
        connectedSamples = sidechainPresent ? std::min(connectedSamples + 1, music.windowSamples()) : 0;

        if (--controlCountdown <= 0)
        {
            controlCountdown = controlInterval;
            meters.voice = voice.measurement();
            meters.music = music.measurement();
            const bool musicSignalValid = !settings.musicSilence.enabled || musicSilence.hasMusicSignal();
            meters.music.reliable = meters.music.reliable && sidechainPresent
                                      && connectedSamples >= music.windowSamples()
                                      && musicSignalValid;
            const bool protectionActive = musicSilence.isProtectionActive(sidechainPresent);
            const bool silencePending = settings.musicSilence.enabled && sidechainPresent
                                        && musicSilence.isMusicActive()
                                        && !musicSilence.hasMusicSignal();
            if (learner.isActive())
            {
                if (protectionActive) gain.setMusicSilenceTarget();
                else gain.setTarget(0.0f);
                meters.boostLimited = meters.cutLimited = false;
                learner.process(gain.gainDb(), meters.voice, meters.music,
                                activity.isReliable(), meters.music.reliable);
                if (learner.getSnapshot().state == LearnState::Completed && !spectralSessionCompleted)
                {
                    spectralSessionCompleted = true;
                    SpectralReference learned;
                    if (spectralLearner.complete(learned))
                        spectralReference = learned;
                }
            }
            else if (protectionActive)
            {
                gain.setMusicSilenceTarget();
                meters.balanceErrorDb = 0.0f;
                meters.boostLimited = meters.cutLimited = false;
                currentMasking = {};
                perceptual.process(0.0f, settings.balance.maxBoostDb, currentMasking,
                                   spectralReference, false, settings.perceptual);
            }
            else if (silencePending)
            {
                // Freeze the last valid target during the hold interval. Do not
                // derive a new correction from a decaying loudness window.
                meters.balanceErrorDb = 0.0f;
                meters.boostLimited = meters.cutLimited = false;
                perceptual.process(0.0f, settings.balance.maxBoostDb, {}, spectralReference,
                                   false, settings.perceptual);
            }
            else if (activity.isReliable() && meters.voice.reliable && meters.music.reliable)
            {
                auto balance = settings.balance;
                const auto learned = learner.getSnapshot();
                if (learned.state == LearnState::Completed
                    && std::abs(balance.offsetDb - learned.referenceDb) > 0.051f)
                    balance.offsetDb = learned.referenceDb;
                const float rawError = meters.music.db + balance.offsetDb - meters.voice.db;
                const float baseTarget = BalanceController::targetGainDb(meters.voice, meters.music,
                                                                          true, balance);
                const float perceptualDb = perceptual.process(baseTarget, balance.maxBoostDb,
                                                              currentMasking, spectralReference,
                                                              true, settings.perceptual);
                const float target = std::clamp(baseTarget + perceptualDb,
                                                -balance.maxCutDb, balance.maxBoostDb);
                gain.setTarget(target);
                meters.balanceErrorDb = rawError;
                meters.boostLimited = rawError - balance.deadbandDb > balance.maxBoostDb;
                meters.cutLimited = rawError + balance.deadbandDb < -balance.maxCutDb;
            }
            else if (!activity.isActive())
            {
                gain.setNeutralTarget();
                meters.boostLimited = meters.cutLimited = false;
            }
            else
            {
                gain.setTarget(0.0f);
                meters.boostLimited = meters.cutLimited = false;
            }
        }
        if (!sidechainPresent || !settings.balance.enabled || learner.isActive())
        {
            gain.setTarget(0.0f);
            meters.boostLimited = meters.cutLimited = false;
        }

        const float linear = gain.nextGain();
        for (int ch = 0; ch < channels; ++ch)
        {
            auto& sample = vocal[static_cast<std::size_t>(ch)];
            // Invalid upstream audio cannot poison the output or detector state.
            const double x = std::isfinite(sample) ? static_cast<double>(sample) * linear : 0.0;
            const double bound = std::numeric_limits<float>::max();
            sample = static_cast<float>(std::clamp(x, -bound, bound));
        }
        meters.gainDb = gain.gainDb();
        meters.targetGainDb = gain.targetGainDb();
        meters.vocalActive = activity.isActive();
        meters.sidechainValid = sidechainPresent && meters.music.reliable;
        meters.musicActive = sidechainPresent && musicSilence.isMusicActive();
        meters.musicSilenceProtectionActive = musicSilence.isProtectionActive(sidechainPresent);
        meters.musicLevelDbfs = musicSilence.getLevelDbfs();
        meters.maskingIndex = currentMasking.index;
        meters.maskingConfidence = currentMasking.confidence;
        meters.perceptualCorrectionDb = perceptual.getCorrectionDb();
        meters.spectralReferenceValid = spectralReference.valid;
        meters.voiceBandsDb = voiceSpectrum.getSnapshot().energyDb;
        meters.musicBandsDb = musicSpectrum.getSnapshot().energyDb;
        meters.competition = currentMasking.competition;
        meters.referenceCompetition = spectralReference.competition;
        meters.learning = learner.getSnapshot();
    }

    MeterSnapshot snapshot() const noexcept { return meters; } // Audio thread only.

private:
    LoudnessDetector voice, music;
    GainSmoother gain;
    VocalActivityDetector activity;
    MusicSilenceDetector musicSilence;
    ReferenceLearner learner;
    SpectralAnalyzer voiceSpectrum, musicSpectrum;
    MaskingEstimate currentMasking;
    SpectralReferenceLearner spectralLearner;
    SpectralReference spectralReference;
    PerceptualBalanceController perceptual;
    EngineSettings settings;
    MeterSnapshot meters;
    std::size_t connectedSamples = 0;
    int channels = 1, controlInterval = 480, controlCountdown = 0;
    bool spectralSessionCompleted = false;
};
}
