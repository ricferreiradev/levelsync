#include "MeterComponents.h"

namespace levelsync::ui
{
void LoudnessMeter::setLevel(float newDb, bool newValid, bool newActive) noexcept
{
    valid = newValid;
    active = newActive;
    const float target = newValid && std::isfinite(newDb) ? juce::jlimit(-70.0f, 6.0f, newDb) : -70.0f;
    displayedDb += 0.22f * (target - displayedDb);
    repaint();
}

void LoudnessMeter::paint(juce::Graphics& g)
{
    auto area = getLocalBounds().toFloat().reduced(12.0f);
    g.setColour(juce::Colour(Colours::muted));
    g.setFont(13.0f);
    g.drawText(name, area.removeFromTop(20.0f), juce::Justification::centredLeft);
    const auto valueArea = area.removeFromTop(30.0f);
    g.setColour(valid ? juce::Colour(Colours::text) : juce::Colour(Colours::muted));
    g.setFont(22.0f);
    g.drawText(valid ? juce::String(displayedDb, 1) + " dB K" : "-- dB K",
               valueArea, juce::Justification::centredLeft);
    auto bar = area.removeFromBottom(10.0f);
    g.setColour(juce::Colour(Colours::background));
    g.fillRoundedRectangle(bar, 5.0f);
    const float proportion = juce::jlimit(0.0f, 1.0f, (displayedDb + 70.0f) / 70.0f);
    bar.setWidth(bar.getWidth() * proportion);
    g.setColour(valid ? meterColour : juce::Colour(Colours::outline));
    g.fillRoundedRectangle(bar, 5.0f);
    g.setColour(active ? juce::Colour(Colours::vocal) : juce::Colour(Colours::muted));
    g.fillEllipse(getWidth() - 23.0f, 15.0f, 8.0f, 8.0f);
}

void BipolarGainMeter::setGain(float db, bool boostLimit, bool cutLimit) noexcept
{
    const float target = std::isfinite(db) ? juce::jlimit(-12.0f, 12.0f, db) : 0.0f;
    displayedDb += 0.28f * (target - displayedDb);
    boostLimited = boostLimit;
    cutLimited = cutLimit;
    repaint();
}

void BipolarGainMeter::paint(juce::Graphics& g)
{
    auto area = getLocalBounds().toFloat().reduced(12.0f);
    g.setColour(juce::Colour(Colours::muted));
    g.setFont(13.0f);
    g.drawText("GAIN CORRECTION", area.removeFromTop(20.0f), juce::Justification::centredLeft);
    g.setColour(juce::Colour(Colours::text));
    g.setFont(22.0f);
    g.drawText((displayedDb >= 0.0f ? "+" : "") + juce::String(displayedDb, 1) + " dB",
               area.removeFromTop(30.0f), juce::Justification::centredLeft);
    auto bar = area.removeFromBottom(12.0f);
    g.setColour(juce::Colour(Colours::background));
    g.fillRoundedRectangle(bar, 6.0f);
    const float centre = bar.getCentreX();
    const float half = bar.getWidth() * 0.5f;
    const float amount = juce::jlimit(-1.0f, 1.0f, displayedDb / 12.0f) * half;
    juce::Rectangle<float> value(amount >= 0.0f ? centre : centre + amount,
                                 bar.getY(), std::abs(amount), bar.getHeight());
    g.setColour(juce::Colour(Colours::correction));
    g.fillRoundedRectangle(value, 6.0f);
    g.setColour(juce::Colour(Colours::text));
    g.drawVerticalLine(static_cast<int>(centre), bar.getY() - 2.0f, bar.getBottom() + 2.0f);
    if (boostLimited || cutLimited)
    {
        g.setColour(juce::Colour(Colours::danger));
        g.setFont(11.0f);
        g.drawText(boostLimited ? "MAX BOOST" : "MAX CUT", area,
                   juce::Justification::centredRight);
    }
}

void BalanceHistory::push(const TelemetryFrame& frame) noexcept
{
    frames[write] = frame;
    write = (write + 1) % capacity;
    count = std::min(count + 1, capacity);
}

void BalanceHistory::paint(juce::Graphics& g)
{
    auto area = getLocalBounds().toFloat().reduced(12.0f);
    g.setColour(juce::Colour(Colours::muted));
    g.setFont(12.0f);
    g.drawText("30 SECOND BALANCE HISTORY", area.removeFromTop(20.0f), juce::Justification::centredLeft);
    auto legend = area.removeFromTop(18.0f);
    g.setColour(juce::Colour(Colours::music)); g.drawText("Instrumental", legend.removeFromLeft(90.0f), juce::Justification::centredLeft);
    g.setColour(juce::Colour(Colours::vocal)); g.drawText("Vocal pre", legend.removeFromLeft(75.0f), juce::Justification::centredLeft);
    g.setColour(juce::Colour(Colours::correction)); g.drawText("Vocal post", legend.removeFromLeft(85.0f), juce::Justification::centredLeft);
    g.setColour(juce::Colours::white.withAlpha(0.65f)); g.drawText("Target", legend, juce::Justification::centredLeft);
    auto plot = area.reduced(0.0f, 5.0f);
    g.setColour(juce::Colour(Colours::background));
    g.fillRoundedRectangle(plot, 5.0f);
    for (int db = -60; db <= 0; db += 10)
    {
        const float y = juce::jmap(static_cast<float>(db), -60.0f, 0.0f, plot.getBottom(), plot.getY());
        g.setColour(juce::Colour(Colours::outline).withAlpha(0.5f));
        g.drawHorizontalLine(static_cast<int>(y), plot.getX(), plot.getRight());
    }
    if (count < 2) return;

    const auto drawCurve = [&](auto value, juce::Colour colour, bool dashed)
    {
        juce::Path path;
        bool started = false;
        for (std::size_t i = 0; i < count; ++i)
        {
            const std::size_t index = (write + capacity - count + i) % capacity;
            const float db = value(frames[index]);
            if (!std::isfinite(db) || db < -100.0f) { started = false; continue; }
            const float x = juce::jmap(static_cast<float>(i), 0.0f, static_cast<float>(capacity - 1),
                                       plot.getX(), plot.getRight());
            const float y = juce::jmap(juce::jlimit(-60.0f, 0.0f, db), -60.0f, 0.0f,
                                       plot.getBottom(), plot.getY());
            if (!started) { path.startNewSubPath(x, y); started = true; } else path.lineTo(x, y);
        }
        g.setColour(colour);
        if (dashed)
        {
            const float dashes[] = {5.0f, 4.0f};
            juce::Path dashedPath;
            juce::PathStrokeType(1.2f).createDashedStroke(dashedPath, path, dashes, 2);
            g.strokePath(dashedPath, juce::PathStrokeType(1.2f));
        }
        else g.strokePath(path, juce::PathStrokeType(1.6f));
    };
    drawCurve([](const auto& f) { return f.musicValid ? f.musicDb : -120.0f; }, juce::Colour(Colours::music), false);
    drawCurve([](const auto& f) { return f.voiceValid ? f.voiceDb : -120.0f; }, juce::Colour(Colours::vocal), false);
    drawCurve([](const auto& f) { return f.voiceValid ? f.outputVoiceDb : -120.0f; }, juce::Colour(Colours::correction), false);
    drawCurve([](const auto& f) { return f.musicValid ? f.musicDb + f.targetBalanceDb : -120.0f; }, juce::Colours::white.withAlpha(0.65f), true);
}

void SpectralGraph::paint(juce::Graphics& g)
{
    auto area = getLocalBounds().toFloat().reduced(8.0f);
    g.setColour(juce::Colour(Colours::background));
    g.fillRoundedRectangle(area, 5.0f);
    const float bandWidth = area.getWidth() / static_cast<float>(spectralBandCount);
    for (std::size_t band = 0; band < spectralBandCount; ++band)
    {
        const float x = area.getX() + static_cast<float>(band) * bandWidth;
        const float voice = juce::jlimit(0.0f, 1.0f, (frame.voiceBandsDb[band] + 80.0f) / 80.0f);
        const float music = juce::jlimit(0.0f, 1.0f, (frame.musicBandsDb[band] + 80.0f) / 80.0f);
        const float competition = juce::jlimit(0.0f, 1.0f, frame.competition[band]);
        const float reference = juce::jlimit(0.0f, 1.0f, frame.referenceCompetition[band]);
        g.setColour(juce::Colour(Colours::music).withAlpha(0.7f));
        g.fillRect(x + 1.0f, area.getBottom() - area.getHeight() * music,
                   bandWidth * 0.42f, area.getHeight() * music);
        g.setColour(juce::Colour(Colours::vocal).withAlpha(0.7f));
        g.fillRect(x + bandWidth * 0.48f, area.getBottom() - area.getHeight() * voice,
                   bandWidth * 0.42f, area.getHeight() * voice);
        if (competition > reference + 0.05f)
        {
            g.setColour(juce::Colour(Colours::correction).withAlpha(0.75f));
            g.fillRect(x + 1.0f, area.getY(), bandWidth - 2.0f, 4.0f + 8.0f * competition);
        }
        g.setColour(juce::Colours::white.withAlpha(0.45f));
        const float y = area.getBottom() - area.getHeight() * reference;
        g.drawHorizontalLine(static_cast<int>(y), x, x + bandWidth - 1.0f);
    }
    g.setColour(juce::Colour(Colours::muted));
    g.setFont(11.0f);
    auto labels = area.toNearestInt().removeFromBottom(18);
    g.drawText("80 Hz", labels.removeFromLeft(55), juce::Justification::centredLeft);
    g.drawText("12 kHz", labels.removeFromRight(55), juce::Justification::centredRight);
}
}
