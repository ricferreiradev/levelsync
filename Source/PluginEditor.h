#pragma once

#include "PluginProcessor.h"
#include "UI/LevelSyncLookAndFeel.h"
#include "UI/MeterComponents.h"

class LevelSyncAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                           private juce::Timer
{
public:
    explicit LevelSyncAudioProcessorEditor(LevelSyncAudioProcessor&);
    ~LevelSyncAudioProcessorEditor() override;
    void paint(juce::Graphics&) override;
    void paintOverChildren(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void configureSlider(std::size_t index, const char* id, const char* label, const char* suffix);
    void drawPanel(juce::Graphics&, juce::Rectangle<int>, const juce::String&) const;

    LevelSyncAudioProcessor& processor;
    levelsync::ui::LevelSyncLookAndFeel lookAndFeel;
    levelsync::ui::LoudnessMeter voiceMeter{"VOCAL LOUDNESS", juce::Colour(levelsync::ui::Colours::vocal)};
    levelsync::ui::LoudnessMeter musicMeter{"INSTRUMENTAL LOUDNESS", juce::Colour(levelsync::ui::Colours::music)};
    levelsync::ui::BipolarGainMeter gainMeter;
    levelsync::ui::BalanceHistory history;
    levelsync::ui::SpectralGraph spectralGraph;

    std::array<juce::Slider, 17> sliders;
    std::array<juce::Label, 17> labels;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>, 17> attachments;
    juce::ComboBox perceptualMode;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> perceptualModeAttachment;
    juce::ToggleButton enable{"Enable"};
    juce::ToggleButton musicSilenceEnable{"Protection Enabled"};
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> enableAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> musicSilenceEnableAttachment;
    juce::TextButton learnButton{"Learn"}, cancelButton{"Cancel"}, diagnosticsButton{"Diagnostics"};
    juce::TextButton licenseButton{"License"};
    double learnProgress = 0.0;
    juce::ProgressBar progressBar{learnProgress};
    bool diagnosticsVisible = false;
    levelsync::TelemetryFrame latestFrame;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LevelSyncAudioProcessorEditor)
};
