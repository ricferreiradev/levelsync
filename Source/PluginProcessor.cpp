#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
constexpr int stateVersion = 6;
constexpr std::array<const char*, 22> parameterIds{
    "offset", "attack", "release", "maxBoost", "maxCut", "strength", "deadband",
    "vocalThreshold", "vocalHold", "silenceReturn", "perceptualMode", "perceptualStrength",
    "maxPerceptualBoost", "maskingResponse", "musicSilenceThreshold", "musicSilenceHold",
    "musicSilenceRelease", "musicSilenceReturn", "musicSilenceEnable", "enable",
    "automationMode", "riderGain"};

juce::String serialiseProfile(const std::array<float, levelsync::spectralBandCount>& profile)
{
    juce::StringArray values;
    for (float value : profile) values.add(juce::String(value, 6));
    return values.joinIntoString(",");
}

bool parseProfile(const juce::String& text,
                  std::array<float, levelsync::spectralBandCount>& profile)
{
    const auto values = juce::StringArray::fromTokens(text, ",", "");
    if (values.size() != static_cast<int>(levelsync::spectralBandCount)) return false;
    for (int i = 0; i < values.size(); ++i)
    {
        const float value = values[i].getFloatValue();
        if (!std::isfinite(value) || value < 0.0f || value > 1.0f) return false;
        profile[static_cast<std::size_t>(i)] = value;
    }
    return true;
}
}

LevelSyncAudioProcessor::LevelSyncAudioProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Voice", juce::AudioChannelSet::mono(), true)
                         .withInput("Instrumental", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::mono(), true)),
      parameters(*this, nullptr, "LevelSyncState", createParameterLayout())
{
    for (std::size_t i = 0; i < parameterIds.size(); ++i)
        parameterValues[i] = parameters.getRawParameterValue(parameterIds[i]);
}

LevelSyncAudioProcessor::~LevelSyncAudioProcessor()
{
    cancelPendingUpdate();
    if (automationGestureActive)
        if (auto* parameter = parameters.getParameter("riderGain"))
            parameter->endChangeGesture();
}

juce::AudioProcessorValueTreeState::ParameterLayout LevelSyncAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    const auto addFloat = [&layout](const char* id, const char* name, float low, float high,
                                    float initial, const char* unit, float step)
    {
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{id, 1}, name, juce::NormalisableRange<float>{low, high, step}, initial,
            juce::AudioParameterFloatAttributes{}.withLabel(unit)));
    };
    addFloat("offset", "Balance Offset", -12.0f, 12.0f, 3.0f, "dB", 0.1f);
    addFloat("attack", "Attack", 50.0f, 1500.0f, 250.0f, "ms", 1.0f);
    addFloat("release", "Release", 100.0f, 3000.0f, 700.0f, "ms", 1.0f);
    addFloat("maxBoost", "Max Boost", 0.0f, 12.0f, 6.0f, "dB", 0.1f);
    addFloat("maxCut", "Max Cut", 0.0f, 12.0f, 6.0f, "dB", 0.1f);
    addFloat("strength", "Strength", 0.0f, 100.0f, 100.0f, "%", 1.0f);
    addFloat("deadband", "Deadband", 0.0f, 2.0f, 0.5f, "dB", 0.05f);
    addFloat("vocalThreshold", "Vocal Threshold", -80.0f, -20.0f, -50.0f, "dBFS", 1.0f);
    addFloat("vocalHold", "Vocal Hold", 0.0f, 2000.0f, 250.0f, "ms", 10.0f);
    addFloat("silenceReturn", "Silence Return", 100.0f, 5000.0f, 1500.0f, "ms", 10.0f);
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{"perceptualMode", 1}, "Perceptual Mode",
        juce::StringArray{"Off", "Gentle", "Balanced", "Strong"}, 0));
    addFloat("perceptualStrength", "Perceptual Strength", 0.0f, 100.0f, 50.0f, "%", 1.0f);
    addFloat("maxPerceptualBoost", "Max Perceptual Boost", 0.0f, 6.0f, 2.0f, "dB", 0.1f);
    addFloat("maskingResponse", "Masking Response", 50.0f, 3000.0f, 500.0f, "ms", 10.0f);
    addFloat("musicSilenceThreshold", "Music Silence Threshold", -90.0f, -20.0f, -55.0f, "dBFS", 1.0f);
    addFloat("musicSilenceHold", "Music Silence Hold", 0.0f, 2000.0f, 200.0f, "ms", 10.0f);
    addFloat("musicSilenceRelease", "Music Silence Release", 0.0f, 3000.0f, 400.0f, "ms", 10.0f);
    addFloat("musicSilenceReturn", "Music Return to Unity", 50.0f, 3000.0f, 300.0f, "ms", 10.0f);
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{"musicSilenceEnable", 1}, "Music Silence Protection", true));
    layout.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"enable", 1}, "Enable", true));
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{"automationMode", 1}, "Automation Mode",
        juce::StringArray{"Off", "Write", "Read"}, 0));
    addFloat("riderGain", "Rider Gain", -12.0f, 12.0f, 0.0f, "dB", 0.01f);
    return layout;
}

void LevelSyncAudioProcessor::prepareToPlay(double sampleRate, int)
{
    engine.prepare(sampleRate, getMainBusNumInputChannels());
    telemetryInterval = std::max(1, static_cast<int>(std::lround(sampleRate / 25.0)));
    telemetryCountdown = telemetryInterval;
    telemetry.discardPending();
    readGainDb.reset(sampleRate, 0.02);
    readGainDb.setCurrentAndTargetValue(0.0f);
    pendingAutomationWrite.store(false, std::memory_order_relaxed);
    lastQueuedRiderGainDb = std::numeric_limits<float>::infinity();
    lastAudioAutomationMode = 0;
    engine.setStoredReference(learnedReferenceDb.load(std::memory_order_acquire),
                              learnedReferenceValid.load(std::memory_order_acquire));
    engine.setSpectralReference(storedSpectralReference);
    learnCompletionPublished = false;
    setLatencySamples(0);
    meters.voiceDb.store(-120.0f, std::memory_order_relaxed);
    meters.musicDb.store(-120.0f, std::memory_order_relaxed);
    meters.gainDb.store(0.0f, std::memory_order_relaxed);
    meters.targetGainDb.store(0.0f, std::memory_order_relaxed);
    meters.balanceErrorDb.store(0.0f, std::memory_order_relaxed);
    meters.vocalActive.store(false, std::memory_order_relaxed);
    meters.sidechainValid.store(false, std::memory_order_relaxed);
    meters.boostLimited.store(false, std::memory_order_relaxed);
    meters.cutLimited.store(false, std::memory_order_relaxed);
    meters.maskingIndex.store(0.0f, std::memory_order_relaxed);
    meters.maskingConfidence.store(0.0f, std::memory_order_relaxed);
    meters.perceptualCorrectionDb.store(0.0f, std::memory_order_relaxed);
    meters.spectralReferenceValid.store(storedSpectralReference.valid, std::memory_order_relaxed);
    meters.musicActive.store(false, std::memory_order_relaxed);
    meters.musicSilenceProtectionActive.store(false, std::memory_order_relaxed);
    meters.musicLevelDbfs.store(-120.0f, std::memory_order_relaxed);
    meters.learnState.store(static_cast<int>(levelsync::LearnState::Idle), std::memory_order_relaxed);
    meters.learnFailure.store(static_cast<int>(levelsync::LearnFailure::None), std::memory_order_relaxed);
    meters.learnProgress.store(0.0f, std::memory_order_relaxed);
    meters.learnValidSeconds.store(0.0f, std::memory_order_relaxed);
}

void LevelSyncAudioProcessor::releaseResources()
{
}

bool LevelSyncAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.inputBuses.size() != 2 || layouts.outputBuses.size() != 1)
        return false;

    const auto mainInput = layouts.getMainInputChannelSet();
    const auto mainOutput = layouts.getMainOutputChannelSet();
    const auto sidechain = layouts.getChannelSet(true, 1);

    return (mainInput == juce::AudioChannelSet::mono()
            || mainInput == juce::AudioChannelSet::stereo())
        && mainOutput == mainInput
        && (sidechain == juce::AudioChannelSet::stereo()
            || sidechain == juce::AudioChannelSet::disabled());
}

void LevelSyncAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                          juce::MidiBuffer& midi)
{
    process(buffer, midi, false);
}

void LevelSyncAudioProcessor::processBlockBypassed(juce::AudioBuffer<float>& buffer,
                                                  juce::MidiBuffer& midi)
{
    process(buffer, midi, true);
}

void LevelSyncAudioProcessor::process(juce::AudioBuffer<float>& buffer,
                                      juce::MidiBuffer& midi, bool bypassed)
{
    juce::ScopedNoDenormals noDenormals;
    midi.clear();

    const auto value = [this](std::size_t i) { return parameterValues[i]->load(std::memory_order_relaxed); };
    levelsync::EngineSettings settings;
    settings.balance.offsetDb = value(0);
    settings.attackMs = value(1);
    settings.releaseMs = value(2);
    settings.balance.maxBoostDb = value(3);
    settings.balance.maxCutDb = value(4);
    settings.balance.strength = value(5) * 0.01f;
    settings.balance.deadbandDb = value(6);
    settings.activity.thresholdDb = value(7);
    settings.activity.holdMs = value(8);
    settings.silenceReturnMs = value(9);
    settings.perceptual.mode = static_cast<levelsync::PerceptualMode>(
        std::clamp(static_cast<int>(std::lround(value(10))), 0, 3));
    settings.perceptual.strength = value(11) * 0.01f;
    settings.perceptual.maxBoostDb = value(12);
    settings.perceptual.responseMs = value(13);
    settings.musicSilence.thresholdDbfs = value(14);
    settings.musicSilence.holdMs = value(15);
    settings.musicSilence.releaseMs = value(16);
    settings.musicSilenceReturnMs = value(17);
    settings.musicSilence.enabled = value(18) >= 0.5f;
    const auto automationMode = static_cast<AutomationMode>(
        std::clamp(static_cast<int>(std::lround(value(20))), 0, 2));
    const bool enabled = !bypassed && value(19) >= 0.5f;
    const bool readAutomation = enabled && automationMode == AutomationMode::read;
    settings.balance.enabled = enabled && !readAutomation;
    engine.setSettings(settings);

    const float riderTargetDb = std::clamp(value(21), -12.0f, 12.0f);
    if (readAutomation)
        readGainDb.setTargetValue(riderTargetDb);
    else
        readGainDb.setCurrentAndTargetValue(riderTargetDb);

    const int modeValue = static_cast<int>(automationMode);
    if (modeValue != lastAudioAutomationMode)
    {
        lastAudioAutomationMode = modeValue;
        triggerAsyncUpdate();
    }

    if (referenceRestorePending.exchange(false, std::memory_order_acq_rel))
    {
        engine.setStoredReference(learnedReferenceDb.load(std::memory_order_acquire),
                                  learnedReferenceValid.load(std::memory_order_acquire));
        engine.setSpectralReference(restoredSpectralReference);
    }

    const int command = learnCommand.exchange(0, std::memory_order_acq_rel);
    if (command == 1)
    {
        engine.startLearning();
        learnCompletionPublished = false;
    }
    else if (command == 2)
        engine.cancelLearning();
    else if (command == 3)
        engine.acknowledgeLearning();

    auto vocal = getBusBuffer(buffer, true, 0);
    const auto instrumental = getBusBuffer(buffer, true, 1);
    const bool sidechainPresent = instrumental.getNumChannels() == 2;
    const int channels = vocal.getNumChannels();
    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        std::array<float, 2> v{};
        std::array<float, 2> originalVocal{};
        std::array<float, 2> m{};
        for (int ch = 0; ch < channels; ++ch)
        {
            v[static_cast<std::size_t>(ch)] = vocal.getSample(ch, sample);
            originalVocal[static_cast<std::size_t>(ch)] = v[static_cast<std::size_t>(ch)];
        }
        if (sidechainPresent)
            for (int ch = 0; ch < 2; ++ch)
                m[static_cast<std::size_t>(ch)] = instrumental.getSample(ch, sample);

        // Measure pre-gain audio, then write exclusively to the main bus.
        engine.processSample(v, m, sidechainPresent);
        float effectiveGainDb = 0.0f;
        if (readAutomation)
        {
            effectiveGainDb = readGainDb.getNextValue();
            const float gain = juce::Decibels::decibelsToGain(effectiveGainDb);
            for (int ch = 0; ch < channels; ++ch)
                v[static_cast<std::size_t>(ch)] = originalVocal[static_cast<std::size_t>(ch)] * gain;
        }
        for (int ch = 0; ch < channels; ++ch)
            vocal.setSample(ch, sample, v[static_cast<std::size_t>(ch)]);

        if (--telemetryCountdown <= 0)
        {
            telemetryCountdown = telemetryInterval;
            const auto live = engine.snapshot();
            levelsync::TelemetryFrame frame;
            frame.voiceDb = live.voice.db;
            frame.musicDb = live.music.db;
            frame.appliedGainDb = readAutomation ? effectiveGainDb : live.gainDb;
            frame.outputVoiceDb = live.voice.db + frame.appliedGainDb;
            frame.currentBalanceDb = live.voice.db - live.music.db;
            frame.targetBalanceDb = settings.balance.offsetDb;
            frame.balanceErrorDb = frame.targetBalanceDb
                                   - (frame.outputVoiceDb - frame.musicDb);
            frame.targetGainDb = live.targetGainDb;
            frame.voiceValid = live.voice.reliable;
            frame.musicValid = live.music.reliable;
            frame.vocalActive = live.vocalActive;
            frame.boostLimited = live.boostLimited;
            frame.cutLimited = live.cutLimited;
            frame.maskingIndex = live.maskingIndex;
            frame.maskingConfidence = live.maskingConfidence;
            frame.perceptualCorrectionDb = live.perceptualCorrectionDb;
            frame.spectralReferenceValid = live.spectralReferenceValid;
            frame.musicActive = live.musicActive;
            frame.musicSilenceProtectionActive = live.musicSilenceProtectionActive;
            frame.musicLevelDbfs = live.musicLevelDbfs;
            frame.voiceBandsDb = live.voiceBandsDb;
            frame.musicBandsDb = live.musicBandsDb;
            frame.competition = live.competition;
            frame.referenceCompetition = live.referenceCompetition;
            telemetry.push(frame);

            if (enabled && automationMode == AutomationMode::write
                && (!std::isfinite(lastQueuedRiderGainDb)
                    || std::abs(live.gainDb - lastQueuedRiderGainDb) >= 0.05f))
            {
                lastQueuedRiderGainDb = live.gainDb;
                pendingRiderGainDb.store(live.gainDb, std::memory_order_relaxed);
                pendingAutomationWrite.store(true, std::memory_order_release);
                triggerAsyncUpdate();
            }
        }
    }

    const auto snapshot = engine.snapshot();
    meters.voiceDb.store(snapshot.voice.db, std::memory_order_relaxed);
    meters.musicDb.store(snapshot.music.db, std::memory_order_relaxed);
    const float displayedGainDb = readAutomation ? readGainDb.getCurrentValue() : snapshot.gainDb;
    meters.gainDb.store(displayedGainDb, std::memory_order_relaxed);
    meters.targetGainDb.store(readAutomation ? riderTargetDb : snapshot.targetGainDb,
                              std::memory_order_relaxed);
    meters.balanceErrorDb.store(snapshot.balanceErrorDb, std::memory_order_relaxed);
    meters.vocalActive.store(snapshot.vocalActive, std::memory_order_relaxed);
    meters.sidechainValid.store(snapshot.sidechainValid, std::memory_order_relaxed);
    meters.boostLimited.store(snapshot.boostLimited, std::memory_order_relaxed);
    meters.cutLimited.store(snapshot.cutLimited, std::memory_order_relaxed);
    meters.maskingIndex.store(snapshot.maskingIndex, std::memory_order_relaxed);
    meters.maskingConfidence.store(snapshot.maskingConfidence, std::memory_order_relaxed);
    meters.perceptualCorrectionDb.store(snapshot.perceptualCorrectionDb, std::memory_order_relaxed);
    meters.spectralReferenceValid.store(snapshot.spectralReferenceValid, std::memory_order_relaxed);
    meters.musicActive.store(snapshot.musicActive, std::memory_order_relaxed);
    meters.musicSilenceProtectionActive.store(snapshot.musicSilenceProtectionActive, std::memory_order_relaxed);
    meters.musicLevelDbfs.store(snapshot.musicLevelDbfs, std::memory_order_relaxed);
    meters.learnState.store(static_cast<int>(snapshot.learning.state), std::memory_order_relaxed);
    meters.learnFailure.store(static_cast<int>(snapshot.learning.failure), std::memory_order_relaxed);
    meters.learnProgress.store(snapshot.learning.progress, std::memory_order_relaxed);
    meters.learnValidSeconds.store(snapshot.learning.validSeconds, std::memory_order_relaxed);

    if (snapshot.learning.state == levelsync::LearnState::Completed && !learnCompletionPublished)
    {
        learnCompletionPublished = true;
        pendingLearnedReference.store(snapshot.learning.referenceDb, std::memory_order_relaxed);
        pendingSpectralReference = engine.getSpectralReference();
        pendingLearnCommit.store(true, std::memory_order_release);
        triggerAsyncUpdate();
    }
}

void LevelSyncAudioProcessor::handleAsyncUpdate()
{
    commitPendingLearningResult();
    serviceAutomationHost();
}

void LevelSyncAudioProcessor::serviceAutomationHost()
{
    auto* parameter = parameters.getParameter("riderGain");
    const auto* modeValue = parameters.getRawParameterValue("automationMode");
    if (parameter == nullptr || modeValue == nullptr)
        return;

    const bool writing = static_cast<int>(std::lround(modeValue->load(std::memory_order_relaxed)))
                         == static_cast<int>(AutomationMode::write);
    if (writing && !automationGestureActive)
    {
        parameter->beginChangeGesture();
        automationGestureActive = true;
    }
    else if (!writing && automationGestureActive)
    {
        parameter->endChangeGesture();
        automationGestureActive = false;
        pendingAutomationWrite.store(false, std::memory_order_release);
    }

    if (writing && pendingAutomationWrite.exchange(false, std::memory_order_acq_rel))
    {
        const float gainDb = std::clamp(pendingRiderGainDb.load(std::memory_order_relaxed),
                                        -12.0f, 12.0f);
        parameter->setValueNotifyingHost(parameter->convertTo0to1(gainDb));
    }
}

void LevelSyncAudioProcessor::commitPendingLearningResult()
{
    if (!pendingLearnCommit.exchange(false, std::memory_order_acq_rel))
        return;

    const float learned = pendingLearnedReference.load(std::memory_order_relaxed);
    learnedReferenceDb.store(learned, std::memory_order_release);
    learnedReferenceValid.store(true, std::memory_order_release);
    storedSpectralReference = pendingSpectralReference;
    if (auto* parameter = parameters.getParameter("offset"))
    {
        parameter->beginChangeGesture();
        parameter->setValueNotifyingHost(parameter->convertTo0to1(learned));
        parameter->endChangeGesture();
    }
    learnCommand.store(3, std::memory_order_release);
}

juce::AudioProcessorEditor* LevelSyncAudioProcessor::createEditor()
{
    return new LevelSyncAudioProcessorEditor(*this);
}

void LevelSyncAudioProcessor::getStateInformation(juce::MemoryBlock& destination)
{
    auto state = parameters.copyState();
    state.setProperty("stateVersion", stateVersion, nullptr);
    state.setProperty("learnedReferenceDb", learnedReferenceDb.load(std::memory_order_acquire), nullptr);
    state.setProperty("hasLearnedReference", learnedReferenceValid.load(std::memory_order_acquire), nullptr);
    state.setProperty("spectralReferenceValid", storedSpectralReference.valid, nullptr);
    state.setProperty("spectralReferenceIndex", storedSpectralReference.index, nullptr);
    state.setProperty("spectralReferenceProfile", serialiseProfile(storedSpectralReference.competition), nullptr);
    if (const auto xml = state.createXml())
        copyXmlToBinary(*xml, destination);
}

void LevelSyncAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (data == nullptr || sizeInBytes <= 0)
        return;
    if (const auto xml = getXmlFromBinary(data, sizeInBytes))
        if (xml->hasTagName(parameters.state.getType()))
        {
            auto state = juce::ValueTree::fromXml(*xml);
            parameters.replaceState(state);
            const int version = static_cast<int>(state.getProperty("stateVersion", 1));
            const bool valid = version >= 2 && static_cast<bool>(state.getProperty("hasLearnedReference", false));
            const float reference = static_cast<float>(state.getProperty("learnedReferenceDb", 3.0f));
            learnedReferenceValid.store(valid && std::isfinite(reference), std::memory_order_release);
            learnedReferenceDb.store(std::isfinite(reference) ? std::clamp(reference, -12.0f, 12.0f) : 3.0f,
                                     std::memory_order_release);
            restoredSpectralReference = {};
            if (version >= 4 && static_cast<bool>(state.getProperty("spectralReferenceValid", false)))
            {
                const float index = static_cast<float>(state.getProperty("spectralReferenceIndex", 0.0f));
                const auto profile = state.getProperty("spectralReferenceProfile").toString();
                if (std::isfinite(index) && index >= 0.0f && index <= 1.0f
                    && parseProfile(profile, restoredSpectralReference.competition))
                {
                    restoredSpectralReference.index = index;
                    restoredSpectralReference.valid = true;
                }
            }
            storedSpectralReference = restoredSpectralReference;
            referenceRestorePending.store(true, std::memory_order_release);
        }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new LevelSyncAudioProcessor();
}
