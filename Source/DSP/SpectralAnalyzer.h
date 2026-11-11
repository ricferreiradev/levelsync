#pragma once

#include <juce_dsp/juce_dsp.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace levelsync
{
constexpr std::size_t spectralBandCount = 24;

struct SpectralSnapshot
{
    std::array<float, spectralBandCount> energy{};
    std::array<float, spectralBandCount> energyDb{};
    std::array<float, spectralBandCount> centreHz{};
    float totalEnergy = 0.0f;
    bool valid = false;
    std::uint64_t generation = 0;
};

// Analysis-only 2048-point FFT with 75% overlap. Audio is copied into fixed
// circular buffers and is never delayed. All storage and filter weights are
// prepared before processing.
class SpectralAnalyzer
{
public:
    static constexpr int fftOrder = 11;
    static constexpr int fftSize = 1 << fftOrder;
    static constexpr int hopSize = fftSize / 4;
    static constexpr int bins = fftSize / 2 + 1;

    SpectralAnalyzer() : fft(fftOrder) {}

    void prepare(double newSampleRate, int newChannels) noexcept
    {
        sampleRate = std::isfinite(newSampleRate) && newSampleRate >= 8000.0 ? newSampleRate : 48000.0;
        channels = std::clamp(newChannels, 1, 2);
        write = samplesUntilAnalysis = 0;
        generation = 0;
        for (auto& channel : ring) channel.fill(0.0f);
        for (int i = 0; i < fftSize; ++i)
        {
            window[static_cast<std::size_t>(i)] = 0.5f - 0.5f * std::cos(
                2.0f * juce::MathConstants<float>::pi * static_cast<float>(i) / static_cast<float>(fftSize - 1));
        }
        buildErbFilterbank();
        snapshot = {};
        snapshot.centreHz = centres;
    }

    bool push(const std::array<float, 2>& samples) noexcept
    {
        for (int ch = 0; ch < channels; ++ch)
        {
            const float x = samples[static_cast<std::size_t>(ch)];
            ring[static_cast<std::size_t>(ch)][write] = std::isfinite(x) ? x : 0.0f;
        }
        write = (write + 1) % fftSize;
        if (++samplesUntilAnalysis < hopSize)
            return false;
        samplesUntilAnalysis = 0;
        analyse();
        return true;
    }

    const SpectralSnapshot& getSnapshot() const noexcept { return snapshot; }

private:
    static float hzToErb(float hz) noexcept { return 21.4f * std::log10(1.0f + 0.00437f * hz); }
    static float erbToHz(float erb) noexcept { return (std::pow(10.0f, erb / 21.4f) - 1.0f) / 0.00437f; }

    void buildErbFilterbank() noexcept
    {
        bandA.fill(-1); bandB.fill(-1); weightA.fill(0.0f); weightB.fill(0.0f);
        normalisers.fill(0.0f);
        const float lowHz = 80.0f;
        const float highHz = std::min(12000.0f, static_cast<float>(sampleRate * 0.45));
        const float lowErb = hzToErb(lowHz), highErb = hzToErb(highHz);
        std::array<float, spectralBandCount + 2> points{};
        for (std::size_t i = 0; i < points.size(); ++i)
            points[i] = erbToHz(lowErb + (highErb - lowErb) * static_cast<float>(i)
                                         / static_cast<float>(points.size() - 1));
        for (std::size_t band = 0; band < spectralBandCount; ++band)
            centres[band] = points[band + 1];
        for (int bin = 0; bin < bins; ++bin)
        {
            const float hz = static_cast<float>(bin * sampleRate / fftSize);
            for (std::size_t band = 0; band < spectralBandCount; ++band)
            {
                float weight = 0.0f;
                if (hz >= points[band] && hz <= points[band + 1])
                    weight = (hz - points[band]) / std::max(1.0f, points[band + 1] - points[band]);
                else if (hz > points[band + 1] && hz <= points[band + 2])
                    weight = (points[band + 2] - hz) / std::max(1.0f, points[band + 2] - points[band + 1]);
                weight = std::max(0.0f, weight);
                if (weight <= 0.0f) continue;
                const auto b = static_cast<std::int8_t>(band);
                if (bandA[static_cast<std::size_t>(bin)] < 0)
                {
                    bandA[static_cast<std::size_t>(bin)] = b;
                    weightA[static_cast<std::size_t>(bin)] = weight;
                }
                else
                {
                    bandB[static_cast<std::size_t>(bin)] = b;
                    weightB[static_cast<std::size_t>(bin)] = weight;
                }
                normalisers[band] += weight;
            }
        }
        for (auto& value : normalisers) value = std::max(value, 1.0f);
    }

    void analyse() noexcept
    {
        snapshot.energy.fill(0.0f);
        double windowPower = 0.0;
        for (float value : window) windowPower += value * value;
        const float normalisation = 1.0f / static_cast<float>(fftSize * windowPower);

        for (int ch = 0; ch < channels; ++ch)
        {
            fftWork.fill(0.0f);
            for (int i = 0; i < fftSize; ++i)
            {
                const auto source = static_cast<std::size_t>((write + i) % fftSize);
                fftWork[static_cast<std::size_t>(i)] = ring[static_cast<std::size_t>(ch)][source]
                                                       * window[static_cast<std::size_t>(i)];
            }
            fft.performFrequencyOnlyForwardTransform(fftWork.data());
            std::array<double, spectralBandCount> sums{};
            for (int bin = 0; bin < bins; ++bin)
            {
                const auto index = static_cast<std::size_t>(bin);
                const float magnitude = fftWork[index];
                const double power = magnitude * magnitude;
                if (bandA[index] >= 0)
                    sums[static_cast<std::size_t>(bandA[index])] += weightA[index] * power;
                if (bandB[index] >= 0)
                    sums[static_cast<std::size_t>(bandB[index])] += weightB[index] * power;
            }
            for (std::size_t band = 0; band < spectralBandCount; ++band)
                snapshot.energy[band] += static_cast<float>(sums[band] / normalisers[band]) * normalisation;
        }

        snapshot.totalEnergy = 0.0f;
        for (std::size_t band = 0; band < spectralBandCount; ++band)
        {
            snapshot.totalEnergy += snapshot.energy[band];
            snapshot.energyDb[band] = 10.0f * std::log10(std::max(snapshot.energy[band], 1.0e-15f));
        }
        snapshot.valid = std::isfinite(snapshot.totalEnergy) && snapshot.totalEnergy > 1.0e-10f;
        snapshot.generation = ++generation;
        snapshot.centreHz = centres;
    }

    juce::dsp::FFT fft;
    std::array<std::array<float, fftSize>, 2> ring{};
    std::array<float, fftSize * 2> fftWork{};
    std::array<float, fftSize> window{};
    std::array<std::int8_t, bins> bandA{}, bandB{};
    std::array<float, bins> weightA{}, weightB{};
    std::array<float, spectralBandCount> centres{}, normalisers{};
    SpectralSnapshot snapshot;
    double sampleRate = 48000.0;
    int channels = 1, write = 0, samplesUntilAnalysis = 0;
    std::uint64_t generation = 0;
};
}
