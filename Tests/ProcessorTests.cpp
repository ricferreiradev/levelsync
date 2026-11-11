#include "PluginProcessor.h"
#include "TestSupport.h"
#include <limits>

namespace
{
void set(LevelSyncAudioProcessor& processor, const char* id, float value)
{
    auto* parameter = processor.parameters.getParameter(id);
    require(parameter != nullptr, "parameter exists");
    parameter->setValueNotifyingHost(parameter->convertTo0to1(value));
}

void configure(LevelSyncAudioProcessor& processor, int mainChannels, bool sidechain)
{
    auto layout = processor.getBusesLayout();
    layout.inputBuses.set(0, mainChannels == 1 ? juce::AudioChannelSet::mono() : juce::AudioChannelSet::stereo());
    layout.inputBuses.set(1, sidechain ? juce::AudioChannelSet::stereo() : juce::AudioChannelSet::disabled());
    layout.outputBuses.set(0, layout.inputBuses[0]);
    require(processor.setBusesLayout(layout), "supported layout accepted");
}
}

int main()
{
    juce::ScopedJuceInitialiser_GUI initialiseJuce;
    int failures = 0;
    run("APVTS defaults, ranges and persistence", []
    {
        LevelSyncAudioProcessor processor;
        require(processor.getParameters().size() == 20, "twenty automated parameters");
        near(processor.parameters.getRawParameterValue("offset")->load(), 3.0, 0.0, "default offset");
        near(processor.parameters.getRawParameterValue("attack")->load(), 250.0, 0.0, "default attack");
        near(processor.parameters.getRawParameterValue("release")->load(), 700.0, 0.0, "default release");
        near(processor.parameters.getRawParameterValue("vocalHold")->load(), 250.0, 0.0, "default vocal hold");
        near(processor.parameters.getRawParameterValue("silenceReturn")->load(), 1500.0, 0.001, "default silence return");
        near(processor.parameters.getRawParameterValue("perceptualMode")->load(), 0.0, 0.0, "perceptual mode defaults off");
        near(processor.parameters.getRawParameterValue("perceptualStrength")->load(), 50.0, 0.0, "perceptual strength default");
        near(processor.parameters.getRawParameterValue("maxPerceptualBoost")->load(), 2.0, 0.0, "perceptual boost default");
        near(processor.parameters.getRawParameterValue("maskingResponse")->load(), 500.0, 0.001, "masking response default");
        near(processor.parameters.getRawParameterValue("musicSilenceThreshold")->load(), -55.0, 0.001,
             "music silence threshold default");
        near(processor.parameters.getRawParameterValue("musicSilenceHold")->load(), 200.0, 0.001,
             "music silence hold default");
        near(processor.parameters.getRawParameterValue("musicSilenceRelease")->load(), 400.0, 0.001,
             "music silence release default");
        near(processor.parameters.getRawParameterValue("musicSilenceReturn")->load(), 300.0, 0.001,
             "music silence return default");
        near(processor.parameters.getRawParameterValue("musicSilenceEnable")->load(), 1.0, 0.0,
             "music silence protection enabled by default");
        set(processor, "offset", -7.3f);
        set(processor, "attack", 1234.0f);
        set(processor, "enable", 0.0f);
        set(processor, "perceptualMode", 2.0f);
        set(processor, "perceptualStrength", 73.0f);
        set(processor, "musicSilenceThreshold", -61.0f);
        juce::MemoryBlock state;
        processor.getStateInformation(state);
        require(state.getSize() > 0, "non-empty state");
        LevelSyncAudioProcessor restored;
        restored.setStateInformation(state.getData(), static_cast<int>(state.getSize()));
        near(restored.parameters.getRawParameterValue("offset")->load(), -7.3, 0.01, "offset restored");
        near(restored.parameters.getRawParameterValue("attack")->load(), 1234.0, 0.01, "attack restored");
        near(restored.parameters.getRawParameterValue("enable")->load(), 0.0, 0.0, "enable restored");
        near(restored.parameters.getRawParameterValue("perceptualMode")->load(), 2.0, 0.0, "perceptual mode restored");
        near(restored.parameters.getRawParameterValue("perceptualStrength")->load(), 73.0, 0.01, "perceptual strength restored");
        near(restored.parameters.getRawParameterValue("musicSilenceThreshold")->load(), -61.0, 0.01,
             "music silence threshold restored");
        restored.setStateInformation(nullptr, 0);
        const char invalid[] = "invalid";
        restored.setStateInformation(invalid, sizeof(invalid));
        near(restored.parameters.getRawParameterValue("offset")->load(), -7.3, 0.01, "invalid state ignored");
    }, failures);

    run("main layouts preserved and auxiliary buffer never changed or mixed", []
    {
        for (const int mainChannels : {1, 2})
        {
            LevelSyncAudioProcessor processor;
            configure(processor, mainChannels, true);
            processor.prepareToPlay(48000.0, 511);
            juce::AudioBuffer<float> buffer(mainChannels + 2, 511);
            juce::MidiBuffer midi;
            for (int block = 0; block < 160; ++block)
            {
                for (int i = 0; i < 511; ++i)
                {
                    for (int ch = 0; ch < mainChannels; ++ch) buffer.setSample(ch, i, 0.0f);
                    buffer.setSample(mainChannels, i, 0.2f);
                    buffer.setSample(mainChannels + 1, i, -0.3f);
                }
                processor.processBlock(buffer, midi);
                for (int i = 0; i < 511; ++i)
                {
                    for (int ch = 0; ch < mainChannels; ++ch)
                        near(buffer.getSample(ch, i), 0.0, 0.0, "no sidechain leakage into output");
                    near(buffer.getSample(mainChannels, i), 0.2f, 0.0, "sidechain left untouched");
                    near(buffer.getSample(mainChannels + 1, i), -0.3f, 0.0, "sidechain right untouched");
                }
            }
            require(processor.getMainBusNumOutputChannels() == mainChannels, "output layout conserved");
            processor.releaseResources();
        }
    }, failures);

    run("processor passthrough without sidechain at supported rates and block sizes", []
    {
        for (const double rate : {44100.0, 48000.0, 96000.0})
            for (const int block : {1, 17, 64, 511, 2048})
            {
                LevelSyncAudioProcessor processor;
                configure(processor, 2, false);
                processor.prepareToPlay(rate, block);
                juce::AudioBuffer<float> buffer(2, block);
                juce::MidiBuffer midi;
                for (int pass = 0; pass < 4; ++pass)
                {
                    for (int i = 0; i < block; ++i)
                    {
                        buffer.setSample(0, i, 0.125f);
                        buffer.setSample(1, i, -0.25f);
                    }
                    processor.processBlock(buffer, midi);
                    for (int i = 0; i < block; ++i)
                    {
                        near(buffer.getSample(0, i), 0.125, 0.0, "left passthrough");
                        near(buffer.getSample(1, i), -0.25, 0.0, "right passthrough");
                    }
                }
                require(!processor.getMeters().sidechainValid.load(), "sidechain warning");
            }
    }, failures);

    run("integration convergence, Enable, Strength and host bypass", []
    {
        LevelSyncAudioProcessor processor;
        configure(processor, 1, true);
        processor.prepareToPlay(48000.0, 257);
        juce::AudioBuffer<float> buffer(3, 257);
        juce::MidiBuffer midi;
        int sample = 0;
        const auto feed = [&](int blocks, bool bypass)
        {
            for (int b = 0; b < blocks; ++b)
            {
                const int firstSample = sample;
                for (int i = 0; i < buffer.getNumSamples(); ++i, ++sample)
                {
                    const float x = 0.05f * std::sin(2.0f * juce::MathConstants<float>::pi * 997.0f
                                                      * static_cast<float>(sample) / 48000.0f);
                    buffer.setSample(0, i, x);
                    buffer.setSample(1, i, x);
                    buffer.setSample(2, i, x);
                }
                if (bypass) processor.processBlockBypassed(buffer, midi);
                else processor.processBlock(buffer, midi);
                for (int i = 0; i < buffer.getNumSamples(); ++i)
                {
                    require(std::isfinite(buffer.getSample(0, i)), "finite processor output");
                    const float expected = 0.05f * std::sin(2.0f * juce::MathConstants<float>::pi * 997.0f
                                                * static_cast<float>(firstSample + i) / 48000.0f);
                    near(buffer.getSample(1, i), expected, 0.0, "active sidechain left unchanged");
                    near(buffer.getSample(2, i), expected, 0.0, "active sidechain right unchanged");
                }
            }
        };
        feed(1200, false);
        near(processor.getMeters().gainDb.load(), 5.5103, 0.025, "default mono/stereo correction");
        require(processor.getMeters().sidechainValid.load(), "valid routed sidechain");
        set(processor, "enable", 0.0f);
        feed(1200, false);
        near(processor.getMeters().gainDb.load(), 0.0, 0.001, "Enable off returns to unity");
        set(processor, "enable", 1.0f);
        set(processor, "strength", 0.0f);
        feed(1200, false);
        near(processor.getMeters().gainDb.load(), 0.0, 0.001, "Strength zero is neutral");
        set(processor, "strength", 100.0f);
        feed(1200, false);
        require(processor.getMeters().gainDb.load() > 5.4f, "re-enabled correction");
        feed(1200, true);
        near(processor.getMeters().gainDb.load(), 0.0, 0.001, "host bypass smooth unity");
        buffer.setSample(0, 0, std::numeric_limits<float>::quiet_NaN());
        buffer.setSample(1, 0, std::numeric_limits<float>::infinity());
        processor.processBlock(buffer, midi);
        require(std::isfinite(buffer.getSample(0, 0)), "invalid main input sanitised");
        require(std::isinf(buffer.getSample(1, 0)), "invalid sidechain is still read-only");
    }, failures);

    run("Learn Balance completes, commits parameter, and persists versioned reference", []
    {
        LevelSyncAudioProcessor processor;
        configure(processor, 1, true);
        processor.prepareToPlay(48000.0, 257);
        juce::AudioBuffer<float> buffer(3, 257);
        juce::MidiBuffer midi;
        int sample = 0;
        const auto feed = [&](int blocks)
        {
            for (int block = 0; block < blocks; ++block)
            {
                for (int i = 0; i < buffer.getNumSamples(); ++i, ++sample)
                {
                    const float voice = 0.1f * std::sin(2.0f * juce::MathConstants<float>::pi * 997.0f
                                                        * static_cast<float>(sample) / 48000.0f);
                    const float music = 0.1f * std::sin(2.0f * juce::MathConstants<float>::pi * 997.0f
                                                        * static_cast<float>(sample) / 48000.0f);
                    buffer.setSample(0, i, voice);
                    buffer.setSample(1, i, music);
                    buffer.setSample(2, i, music);
                }
                processor.processBlock(buffer, midi);
            }
        };

        feed(800);
        require(std::abs(processor.getMeters().gainDb.load()) > 0.5f, "correction active before learn");
        processor.requestLearnBalance();
        feed(1600);
        require(static_cast<levelsync::LearnState>(processor.getMeters().learnState.load())
                    == levelsync::LearnState::Completed,
                "processor learning completed");
        near(processor.getMeters().learnProgress.load(), 1.0, 0.0, "processor progress complete");
        processor.commitPendingLearningResult();
        require(processor.hasLearnedReference(), "learned reference committed");
        require(processor.hasSpectralReference(), "spectral reference committed");
        near(processor.getLearnedReferenceDb(), -3.0103, 0.06, "learned pre-gain relation");
        near(processor.parameters.getRawParameterValue("offset")->load(), -3.0, 0.051,
             "manual reference parameter updated");

        set(processor, "offset", 2.0f);
        feed(1200);
        near(processor.getMeters().gainDb.load(), 4.5103, 0.03,
             "manual adjustment overrides learned base after commit");

        juce::MemoryBlock state;
        processor.getStateInformation(state);
        LevelSyncAudioProcessor restored;
        restored.setStateInformation(state.getData(), static_cast<int>(state.getSize()));
        require(restored.hasLearnedReference(), "learned flag restored");
        require(restored.hasSpectralReference(), "spectral reference restored");
        near(restored.getLearnedReferenceDb(), processor.getLearnedReferenceDb(), 1.0e-6,
             "learned reference restored");
        near(restored.parameters.getRawParameterValue("offset")->load(), 2.0, 0.051,
             "manual post-learn offset restored separately");
    }, failures);

    run("version 3 state remains compatible and has no spectral calibration", []
    {
        LevelSyncAudioProcessor source;
        juce::MemoryBlock current;
        source.getStateInformation(current);
        auto xml = juce::AudioProcessor::getXmlFromBinary(current.getData(), static_cast<int>(current.getSize()));
        require(xml != nullptr, "state XML parses");
        auto state = juce::ValueTree::fromXml(*xml);
        state.setProperty("stateVersion", 3, nullptr);
        state.removeProperty("spectralReferenceValid", nullptr);
        state.removeProperty("spectralReferenceIndex", nullptr);
        state.removeProperty("spectralReferenceProfile", nullptr);
        std::unique_ptr<juce::XmlElement> legacyXml(state.createXml());
        juce::MemoryBlock legacy;
        juce::AudioProcessor::copyXmlToBinary(*legacyXml, legacy);

        LevelSyncAudioProcessor restored;
        restored.setStateInformation(legacy.getData(), static_cast<int>(legacy.getSize()));
        require(!restored.hasSpectralReference(), "legacy state does not invent spectral profile");
        near(restored.parameters.getRawParameterValue("perceptualMode")->load(), 0.0, 0.0,
             "legacy state keeps safe perceptual default");
    }, failures);

    run("editor constructs and renders to an offscreen image", []
    {
        LevelSyncAudioProcessor processor;
        require(processor.hasEditor(), "editor available");
        std::unique_ptr<juce::AudioProcessorEditor> editor(processor.createEditor());
        require(editor != nullptr && editor->getWidth() == 1180 && editor->getHeight() == 820, "editor size");
        const auto image = editor->createComponentSnapshot(editor->getLocalBounds());
        require(image.isValid(), "editor snapshot renders");
        const auto path = juce::File::getCurrentWorkingDirectory().getChildFile("LevelSync-editor.png");
        juce::FileOutputStream output(path);
        require(output.openedOk(), "preview output opens");
        output.setPosition(0);
        output.truncate();
        require(juce::PNGImageFormat{}.writeImageToStream(image, output), "preview saved");
    }, failures);

    run("editor repeatedly opens, resizes, closes, and DSP continues headless", []
    {
        LevelSyncAudioProcessor processor;
        configure(processor, 1, true);
        processor.prepareToPlay(48000.0, 64);
        for (int iteration = 0; iteration < 12; ++iteration)
        {
            std::unique_ptr<juce::AudioProcessorEditor> editor(processor.createEditor());
            const int width = 900 + (iteration % 4) * 150;
            const int height = 680 + (iteration % 3) * 100;
            editor->setSize(width, height);
            const auto image = editor->createComponentSnapshot(editor->getLocalBounds());
            require(image.isValid() && image.getWidth() == width && image.getHeight() == height,
                    "resized editor renders");
        }
        juce::AudioBuffer<float> buffer(3, 64);
        juce::MidiBuffer midi;
        for (int block = 0; block < 1000; ++block)
        {
            buffer.clear();
            processor.processBlock(buffer, midi);
            for (int i = 0; i < buffer.getNumSamples(); ++i)
                require(std::isfinite(buffer.getSample(0, i)), "headless DSP remains finite");
        }
        juce::AudioBuffer<float> empty(3, 0);
        processor.processBlock(empty, midi);
    }, failures);

    std::cout << "Processor failures: " << failures << '\n';
    return failures == 0 ? 0 : 1;
}
