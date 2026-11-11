#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "DSP/BalanceEngine.h"
#include "DSP/TelemetryQueue.h"
#include <atomic>

class LevelSyncAudioProcessor final : public juce::AudioProcessor,
                                      private juce::AsyncUpdater
{
public:
    LevelSyncAudioProcessor();

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) override;
    void processBlockBypassed(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) override;
    using juce::AudioProcessor::processBlock;
    using juce::AudioProcessor::processBlockBypassed;

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    bool hasEditor() const override { return true; }
    juce::AudioProcessorEditor* createEditor() override;

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destination) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState parameters;

    struct Meters
    {
        std::atomic<float> voiceDb{-120.0f}, musicDb{-120.0f}, gainDb{0.0f};
        std::atomic<float> targetGainDb{0.0f}, balanceErrorDb{0.0f};
        std::atomic<bool> vocalActive{false}, sidechainValid{false};
        std::atomic<bool> boostLimited{false}, cutLimited{false};
        std::atomic<float> maskingIndex{0.0f}, maskingConfidence{0.0f}, perceptualCorrectionDb{0.0f};
        std::atomic<bool> spectralReferenceValid{false};
        std::atomic<bool> musicActive{false}, musicSilenceProtectionActive{false};
        std::atomic<float> musicLevelDbfs{-120.0f};
        std::atomic<int> learnState{static_cast<int>(levelsync::LearnState::Idle)};
        std::atomic<int> learnFailure{static_cast<int>(levelsync::LearnFailure::None)};
        std::atomic<float> learnProgress{0.0f}, learnValidSeconds{0.0f};
    };
    const Meters& getMeters() const noexcept { return meters; }
    void requestLearnBalance() noexcept { learnCommand.store(1, std::memory_order_release); }
    void requestCancelLearning() noexcept { learnCommand.store(2, std::memory_order_release); }
    bool hasLearnedReference() const noexcept { return learnedReferenceValid.load(std::memory_order_acquire); }
    bool hasSpectralReference() const noexcept { return storedSpectralReference.valid; }
    float getLearnedReferenceDb() const noexcept { return learnedReferenceDb.load(std::memory_order_acquire); }
    // Message-thread hook used by the editor timer and headless hosts/tests.
    void commitPendingLearningResult();
    bool popTelemetry(levelsync::TelemetryFrame& frame) noexcept { return telemetry.pop(frame); }
    void discardPendingTelemetry() noexcept { telemetry.discardPending(); }

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    void process(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi, bool bypassed);
    void handleAsyncUpdate() override;
    levelsync::BalanceEngine engine;
    Meters meters;
    // Cached once; no map lookups or ValueTree access on the audio thread.
    std::array<std::atomic<float>*, 20> parameterValues{};
    levelsync::TelemetryQueue<512> telemetry;
    int telemetryCountdown = 1, telemetryInterval = 1920;
    std::atomic<int> learnCommand{0};
    std::atomic<float> learnedReferenceDb{3.0f};
    std::atomic<bool> learnedReferenceValid{false};
    std::atomic<float> pendingLearnedReference{3.0f};
    std::atomic<bool> pendingLearnCommit{false};
    std::atomic<bool> referenceRestorePending{false};
    levelsync::SpectralReference storedSpectralReference;
    levelsync::SpectralReference pendingSpectralReference;
    levelsync::SpectralReference restoredSpectralReference;
    bool learnCompletionPublished = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LevelSyncAudioProcessor)
};
