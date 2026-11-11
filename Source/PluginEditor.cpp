#include "PluginEditor.h"

namespace
{
constexpr auto background = levelsync::ui::Colours::background;
constexpr auto panel = levelsync::ui::Colours::panel;
constexpr auto panelRaised = levelsync::ui::Colours::panelRaised;
constexpr auto outline = levelsync::ui::Colours::outline;
constexpr auto text = levelsync::ui::Colours::text;
constexpr auto muted = levelsync::ui::Colours::muted;
constexpr auto vocal = levelsync::ui::Colours::vocal;
constexpr auto music = levelsync::ui::Colours::music;
constexpr auto correction = levelsync::ui::Colours::correction;
}

LevelSyncAudioProcessorEditor::LevelSyncAudioProcessorEditor(LevelSyncAudioProcessor& owner)
    : AudioProcessorEditor(owner), processor(owner)
{
    setLookAndFeel(&lookAndFeel);
    configureSlider(0, "offset", "Target Balance", " dB");
    configureSlider(1, "strength", "Strength", " %");
    configureSlider(2, "maxBoost", "Max Boost", " dB");
    configureSlider(3, "maxCut", "Max Cut", " dB");
    configureSlider(4, "attack", "Attack", " ms");
    configureSlider(5, "release", "Release", " ms");
    configureSlider(6, "deadband", "Deadband", " dB");
    configureSlider(7, "vocalThreshold", "Activity Threshold", " dBFS");
    configureSlider(8, "vocalHold", "Vocal Hold", " ms");
    configureSlider(9, "silenceReturn", "Silence Return", " ms");
    configureSlider(10, "perceptualStrength", "Strength", " %");
    configureSlider(11, "maxPerceptualBoost", "Max Boost", " dB");
    configureSlider(12, "maskingResponse", "Response", " ms");
    configureSlider(13, "musicSilenceThreshold", "Threshold", " dBFS");
    configureSlider(14, "musicSilenceHold", "Silence Hold", " ms");
    configureSlider(15, "musicSilenceRelease", "Silence Release", " ms");
    configureSlider(16, "musicSilenceReturn", "Return to Unity", " ms");

    for (auto* component : std::array<juce::Component*, 11>{
             &voiceMeter, &musicMeter, &gainMeter, &history, &enable,
             &musicSilenceEnable, &learnButton, &cancelButton, &diagnosticsButton,
             &licenseButton, &progressBar})
        addAndMakeVisible(*component);
    addChildComponent(spectralGraph);
    addAndMakeVisible(perceptualMode);
    perceptualMode.addItemList({"Off", "Gentle", "Balanced", "Strong"}, 1);
    perceptualModeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        processor.parameters, "perceptualMode", perceptualMode);

    enableAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        processor.parameters, "enable", enable);
    musicSilenceEnableAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        processor.parameters, "musicSilenceEnable", musicSilenceEnable);
    learnButton.onClick = [this] { processor.requestLearnBalance(); };
    cancelButton.onClick = [this] { processor.requestCancelLearning(); };
    licenseButton.onClick = []
    {
        juce::AlertWindow::showMessageBoxAsync(
            juce::MessageBoxIconType::InfoIcon,
            "LevelSync | License",
            "Copyright (C) 2026 LevelSync contributors\n\n"
            "Licensed under the GNU Affero General Public License, version 3 "
            "or (at your option) any later version.\n\n"
            "This software is provided without warranty. The corresponding source, "
            "complete license and third-party notices are provided in the LevelSync "
            "source distribution: LICENSE, README.md and THIRD_PARTY_NOTICES.md.");
    };
    diagnosticsButton.setClickingTogglesState(true);
    diagnosticsButton.onClick = [this]
    {
        diagnosticsVisible = diagnosticsButton.getToggleState();
        spectralGraph.setVisible(diagnosticsVisible);
        repaint();
    };
    cancelButton.setEnabled(false);
    progressBar.setPercentageDisplay(true);
    processor.discardPendingTelemetry();
    setResizable(true, true);
    setResizeLimits(820, 640, 1400, 950);
    setSize(1000, 700);
    startTimerHz(25);
}

LevelSyncAudioProcessorEditor::~LevelSyncAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel(nullptr);
}

void LevelSyncAudioProcessorEditor::configureSlider(std::size_t index, const char* id,
                                                     const char* label, const char* suffix)
{
    addAndMakeVisible(sliders[index]);
    addAndMakeVisible(labels[index]);
    sliders[index].setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    sliders[index].setTextBoxStyle(juce::Slider::TextBoxBelow, false, 70, 19);
    sliders[index].setTextValueSuffix(suffix);
    labels[index].setText(label, juce::dontSendNotification);
    labels[index].setJustificationType(juce::Justification::centred);
    labels[index].setFont(juce::FontOptions(10.5f));
    attachments[index] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processor.parameters, id, sliders[index]);
}

void LevelSyncAudioProcessorEditor::timerCallback()
{
    processor.commitPendingLearningResult();
    levelsync::TelemetryFrame frame;
    while (processor.popTelemetry(frame))
    {
        latestFrame = frame;
        history.push(frame);
        spectralGraph.setFrame(frame);
    }

    const auto& meters = processor.getMeters();
    learnProgress = meters.learnProgress.load(std::memory_order_relaxed);
    const auto learnState = static_cast<levelsync::LearnState>(meters.learnState.load(std::memory_order_relaxed));
    const bool learning = learnState == levelsync::LearnState::Preparing
                          || learnState == levelsync::LearnState::Learning;
    learnButton.setEnabled(!learning);
    cancelButton.setEnabled(learning);
    voiceMeter.setLevel(meters.voiceDb.load(std::memory_order_relaxed),
                        latestFrame.voiceValid, meters.vocalActive.load(std::memory_order_relaxed));
    musicMeter.setLevel(meters.musicDb.load(std::memory_order_relaxed),
                        meters.sidechainValid.load(std::memory_order_relaxed),
                        meters.sidechainValid.load(std::memory_order_relaxed));
    gainMeter.setGain(meters.gainDb.load(std::memory_order_relaxed),
                      meters.boostLimited.load(std::memory_order_relaxed),
                      meters.cutLimited.load(std::memory_order_relaxed));
    repaint();
}

void LevelSyncAudioProcessorEditor::drawPanel(juce::Graphics& g, juce::Rectangle<int> bounds,
                                               const juce::String& title) const
{
    g.setColour(juce::Colour(panel));
    g.fillRoundedRectangle(bounds.toFloat(), 10.0f);
    g.setColour(juce::Colour(outline));
    g.drawRoundedRectangle(bounds.toFloat().reduced(0.5f), 10.0f, 1.0f);
    if (title.isNotEmpty())
    {
        g.setColour(juce::Colour(muted));
        g.setFont(12.0f);
        g.drawText(title.toUpperCase(), bounds.reduced(14, 8).removeFromTop(18),
                   juce::Justification::centredLeft);
    }
}

void LevelSyncAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(background));
    auto bounds = getLocalBounds().reduced(12);
    auto header = bounds.removeFromTop(38);
    g.setColour(juce::Colour(text));
    g.setFont(juce::FontOptions(24.0f, juce::Font::bold));
    g.drawText("LevelSync", header.removeFromLeft(155), juce::Justification::centredLeft);
    g.setColour(juce::Colour(muted));
    g.setFont(11.5f);
    g.drawText("VOCAL BALANCE PROCESSOR  |  K-WEIGHTED ESTIMATES",
               header, juce::Justification::centredLeft);

    const int gap = 8;
    auto monitor = bounds.removeFromTop(90);
    const int cardWidth = (monitor.getWidth() - 4 * gap) / 5;
    auto voiceCard = monitor.removeFromLeft(cardWidth); monitor.removeFromLeft(gap);
    auto musicCard = monitor.removeFromLeft(cardWidth); monitor.removeFromLeft(gap);
    auto currentCard = monitor.removeFromLeft(cardWidth); monitor.removeFromLeft(gap);
    auto targetCard = monitor.removeFromLeft(cardWidth); monitor.removeFromLeft(gap);
    auto gainCard = monitor;
    drawPanel(g, voiceCard, {}); drawPanel(g, musicCard, {}); drawPanel(g, currentCard, {});
    drawPanel(g, targetCard, {}); drawPanel(g, gainCard, {});

    const auto drawValueCard = [&](juce::Rectangle<int> card, const juce::String& title,
                                   const juce::String& value, juce::Colour colour)
    {
        auto a = card.reduced(10);
        g.setColour(juce::Colour(muted)); g.setFont(11.0f);
        g.drawText(title, a.removeFromTop(18), juce::Justification::centredLeft);
        g.setColour(colour); g.setFont(juce::FontOptions(19.0f, juce::Font::bold));
        g.drawText(value, a.removeFromTop(28), juce::Justification::centredLeft);
    };
    const bool balanceValid = latestFrame.voiceValid && latestFrame.musicValid;
    drawValueCard(currentCard, "CURRENT BALANCE",
                  balanceValid ? juce::String(latestFrame.currentBalanceDb, 1) + " dB" : "-- dB",
                  juce::Colour(text));
    const float targetDb = processor.parameters.getRawParameterValue("offset")->load(std::memory_order_relaxed);
    drawValueCard(targetCard, "TARGET BALANCE", juce::String(targetDb, 1) + " dB",
                  juce::Colour(vocal));

    bounds.removeFromTop(gap);
    auto middle = bounds.removeFromTop(190);
    auto graphPanel = middle.removeFromLeft(static_cast<int>(middle.getWidth() * 0.68f));
    middle.removeFromLeft(gap);
    auto learnPanel = middle;
    drawPanel(g, graphPanel, {});
    const auto learnState = static_cast<levelsync::LearnState>(
        processor.getMeters().learnState.load(std::memory_order_relaxed));
    const bool learning = learnState == levelsync::LearnState::Preparing
                          || learnState == levelsync::LearnState::Learning;
    g.setColour(learning ? juce::Colour(vocal).withAlpha(0.18f) : juce::Colour(panel));
    g.fillRoundedRectangle(learnPanel.toFloat(), 10.0f);
    g.setColour(learning ? juce::Colour(vocal) : juce::Colour(outline));
    g.drawRoundedRectangle(learnPanel.toFloat().reduced(0.5f), 10.0f, learning ? 2.0f : 1.0f);
    auto learnText = learnPanel.reduced(12, 8);
    g.setColour(juce::Colour(text)); g.setFont(juce::FontOptions(15.0f, juce::Font::bold));
    g.drawText("Learn Balance", learnText.removeFromTop(21), juce::Justification::centredLeft);
    const auto stateName = [learnState]
    {
        switch (learnState)
        {
            case levelsync::LearnState::Preparing: return juce::String("Preparing | returning to unity");
            case levelsync::LearnState::Learning: return juce::String("Learning valid audio");
            case levelsync::LearnState::Completed: return juce::String("Completed");
            case levelsync::LearnState::Failed: return juce::String("Failed | insufficient valid audio");
            case levelsync::LearnState::Cancelled: return juce::String("Cancelled");
            case levelsync::LearnState::Idle: return juce::String("Ready");
        }
        return juce::String("Ready");
    }();
    g.setColour(juce::Colour(muted)); g.setFont(11.5f);
    g.drawText(stateName, learnText.removeFromTop(18), juce::Justification::centredLeft);
    g.drawText("Valid audio  " + juce::String(processor.getMeters().learnValidSeconds.load(), 1) + " / 5.0 s",
               learnText.removeFromTop(18), juce::Justification::centredLeft);
    g.setColour(processor.hasLearnedReference() ? juce::Colour(vocal) : juce::Colour(muted));
    g.drawText(processor.hasLearnedReference()
                   ? "Learned  " + juce::String(processor.getLearnedReferenceDb(), 1) + " dB"
                   : "No learned reference",
               learnText.removeFromTop(18), juce::Justification::centredLeft);
    if (learning && (!latestFrame.vocalActive || !latestFrame.musicValid))
    {
        g.setColour(juce::Colour(correction));
        g.drawText(!latestFrame.vocalActive ? "Waiting for vocal activity" : "Waiting for sidechain",
                   learnText.removeFromTop(18), juce::Justification::centredLeft);
    }

    bounds.removeFromTop(gap);
    auto controls = bounds;
    auto protectionPanel = controls.removeFromBottom(130);
    controls.removeFromBottom(gap);
    auto basic = controls.removeFromLeft(static_cast<int>(controls.getWidth() * 0.30f));
    controls.removeFromLeft(gap);
    auto advanced = controls.removeFromLeft(static_cast<int>(controls.getWidth() * 0.61f));
    controls.removeFromLeft(gap);
    auto perceptualPanel = controls;
    drawPanel(g, basic, "Basic"); drawPanel(g, advanced, "Advanced");
    drawPanel(g, perceptualPanel, "Perceptual Balance");
    drawPanel(g, protectionPanel, "Music Silence Protection");
    auto protectionStatus = protectionPanel.reduced(14, 8).removeFromTop(18);
    protectionStatus.removeFromLeft(210);
    const auto& silenceMeters = processor.getMeters();
    const bool protectionActive = silenceMeters.musicSilenceProtectionActive.load(std::memory_order_relaxed);
    const bool musicIsActive = silenceMeters.musicActive.load(std::memory_order_relaxed);
    g.setColour(protectionActive ? juce::Colour(correction) : juce::Colour(vocal));
    g.setFont(juce::FontOptions(12.0f, juce::Font::bold));
    g.drawText(musicIsActive ? "MUSIC ACTIVE" : "MUSIC SILENT",
               protectionStatus.removeFromLeft(120), juce::Justification::centredLeft);
    g.drawText(protectionActive ? "PROTECTION ACTIVE" : "PROTECTION IDLE",
               protectionStatus, juce::Justification::centredLeft);

    if (diagnosticsVisible)
    {
        auto diag = getLocalBounds().reduced(32);
        diag = diag.withSizeKeepingCentre(juce::jmin(680, diag.getWidth()), 460);
        g.setColour(juce::Colour(panelRaised).withAlpha(0.98f));
        g.fillRoundedRectangle(diag.toFloat(), 10.0f);
        g.setColour(juce::Colour(vocal));
        g.drawRoundedRectangle(diag.toFloat(), 10.0f, 1.5f);
        auto d = diag.reduced(16);
        g.setFont(juce::FontOptions(17.0f, juce::Font::bold));
        g.drawText("Algorithm Diagnostics", d.removeFromTop(28), juce::Justification::centredLeft);
        g.setColour(juce::Colour(text)); g.setFont(13.0f);
        const auto row = [&](const juce::String& left, const juce::String& right)
        {
            auto r = d.removeFromTop(24);
            g.drawText(left, r.removeFromLeft(r.getWidth() / 2), juce::Justification::centredLeft);
            g.drawText(right, r, juce::Justification::centredLeft);
        };
        row("Vocal pre  " + juce::String(latestFrame.voiceDb, 1) + " dB K",
            "Music  " + juce::String(latestFrame.musicDb, 1) + " dB K");
        row("Current  " + juce::String(latestFrame.currentBalanceDb, 1) + " dB",
            "Target  " + juce::String(latestFrame.targetBalanceDb, 1) + " dB");
        row("Error  " + juce::String(latestFrame.balanceErrorDb, 1) + " dB",
            "Target gain  " + juce::String(latestFrame.targetGainDb, 1) + " dB");
        row("Applied  " + juce::String(latestFrame.appliedGainDb, 1) + " dB",
            juce::String("Vocal ") + (latestFrame.vocalActive ? "active" : "inactive"));
        row(juce::String("Sidechain ") + (latestFrame.musicValid ? "valid" : "invalid"),
            latestFrame.boostLimited ? "Limited by Max Boost"
                                     : (latestFrame.cutLimited ? "Limited by Max Cut" : "No gain limit"));
        row("Masking  " + juce::String(latestFrame.maskingIndex, 2)
                + "  confidence " + juce::String(latestFrame.maskingConfidence, 2),
            "Perceptual  +" + juce::String(latestFrame.perceptualCorrectionDb, 2) + " dB");
    }
}

void LevelSyncAudioProcessorEditor::paintOverChildren(juce::Graphics& g)
{
    if (!diagnosticsVisible)
        return;

    // Component children are painted after paint(). Redraw the diagnostics panel
    // here so it remains a true modal overlay above every knob, label and meter.
    auto diag = getLocalBounds().reduced(32);
    diag = diag.withSizeKeepingCentre(juce::jmin(680, diag.getWidth()), 460);
    g.setColour(juce::Colour(panelRaised));
    g.fillRoundedRectangle(diag.toFloat(), 10.0f);
    g.setColour(juce::Colour(vocal));
    g.drawRoundedRectangle(diag.toFloat(), 10.0f, 1.5f);

    auto d = diag.reduced(16);
    g.setFont(juce::FontOptions(17.0f, juce::Font::bold));
    g.drawText("Algorithm Diagnostics", d.removeFromTop(28), juce::Justification::centredLeft);
    g.setColour(juce::Colour(text));
    g.setFont(13.0f);
    const auto row = [&](const juce::String& left, const juce::String& right)
    {
        auto r = d.removeFromTop(24);
        g.drawText(left, r.removeFromLeft(r.getWidth() / 2), juce::Justification::centredLeft);
        g.drawText(right, r, juce::Justification::centredLeft);
    };
    row("Vocal pre  " + juce::String(latestFrame.voiceDb, 1) + " dB K",
        "Music  " + juce::String(latestFrame.musicDb, 1) + " dB K");
    row("Current  " + juce::String(latestFrame.currentBalanceDb, 1) + " dB",
        "Target  " + juce::String(latestFrame.targetBalanceDb, 1) + " dB");
    row("Error  " + juce::String(latestFrame.balanceErrorDb, 1) + " dB",
        "Target gain  " + juce::String(latestFrame.targetGainDb, 1) + " dB");
    row("Applied  " + juce::String(latestFrame.appliedGainDb, 1) + " dB",
        juce::String("Vocal ") + (latestFrame.vocalActive ? "active" : "inactive"));
    row(juce::String("Sidechain ") + (latestFrame.musicValid ? "valid" : "invalid"),
        latestFrame.boostLimited ? "Limited by Max Boost"
                                 : (latestFrame.cutLimited ? "Limited by Max Cut" : "No gain limit"));
    row("Masking  " + juce::String(latestFrame.maskingIndex, 2)
            + "  confidence " + juce::String(latestFrame.maskingConfidence, 2),
        "Perceptual  +" + juce::String(latestFrame.perceptualCorrectionDb, 2) + " dB");

    // The graph is normally a child component, so render it once more after the
    // overlay background to keep it in the same topmost visual layer.
    const auto graphBounds = spectralGraph.getBounds();
    g.saveState();
    g.reduceClipRegion(graphBounds);
    g.setOrigin(graphBounds.getPosition());
    spectralGraph.paint(g);
    g.restoreState();
}

void LevelSyncAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds().reduced(12);
    auto header = bounds.removeFromTop(38);
    enable.setBounds(header.removeFromRight(90).reduced(2, 8));
    diagnosticsButton.setBounds(header.removeFromRight(105).reduced(2, 8));
    licenseButton.setBounds(header.removeFromRight(78).reduced(2, 8));

    const int gap = 8;
    auto monitor = bounds.removeFromTop(90);
    const int cardWidth = (monitor.getWidth() - 4 * gap) / 5;
    voiceMeter.setBounds(monitor.removeFromLeft(cardWidth)); monitor.removeFromLeft(gap);
    musicMeter.setBounds(monitor.removeFromLeft(cardWidth)); monitor.removeFromLeft(gap);
    monitor.removeFromLeft(cardWidth + gap); // Current Balance is painted.
    monitor.removeFromLeft(cardWidth + gap); // Target Balance is painted.
    gainMeter.setBounds(monitor);

    bounds.removeFromTop(gap);
    auto middle = bounds.removeFromTop(190);
    auto graphPanel = middle.removeFromLeft(static_cast<int>(middle.getWidth() * 0.68f));
    middle.removeFromLeft(gap);
    auto learnPanel = middle;
    history.setBounds(graphPanel);
    auto learnControls = learnPanel.reduced(12, 8);
    learnControls.removeFromTop(82);
    progressBar.setBounds(learnControls.removeFromTop(20));
    learnControls.removeFromTop(6);
    auto buttons = learnControls.removeFromTop(28);
    learnButton.setBounds(buttons.removeFromLeft((buttons.getWidth() - 8) / 2));
    buttons.removeFromLeft(8);
    cancelButton.setBounds(buttons);

    bounds.removeFromTop(gap);
    auto controls = bounds;
    auto protectionPanel = controls.removeFromBottom(130);
    controls.removeFromBottom(gap);
    auto basic = controls.removeFromLeft(static_cast<int>(controls.getWidth() * 0.30f));
    controls.removeFromLeft(gap);
    auto advanced = controls.removeFromLeft(static_cast<int>(controls.getWidth() * 0.61f));
    controls.removeFromLeft(gap);
    auto perceptualPanel = controls;
    basic.reduce(8, 24);
    advanced.reduce(8, 24);
    const auto layoutKnobs = [this](juce::Rectangle<int> area, std::size_t first, std::size_t count)
    {
        const int width = area.getWidth() / static_cast<int>(count);
        for (std::size_t i = 0; i < count; ++i)
        {
            auto cell = area.removeFromLeft(width);
            labels[first + i].setBounds(cell.removeFromTop(17));
            sliders[first + i].setBounds(cell.reduced(2));
        }
    };
    layoutKnobs(basic, 0, 4);
    layoutKnobs(advanced, 4, 6);
    perceptualPanel.reduce(8, 24);
    perceptualMode.setBounds(perceptualPanel.removeFromTop(24));
    perceptualPanel.removeFromTop(2);
    layoutKnobs(perceptualPanel, 10, 3);

    protectionPanel.reduce(10, 26);
    auto protectionToggle = protectionPanel.removeFromLeft(170);
    musicSilenceEnable.setBounds(protectionToggle.removeFromTop(28));
    layoutKnobs(protectionPanel, 13, 4);

    auto diag = getLocalBounds().reduced(32);
    diag = diag.withSizeKeepingCentre(juce::jmin(680, diag.getWidth()), 460).reduced(16);
    diag.removeFromTop(190);
    spectralGraph.setBounds(diag);
}
