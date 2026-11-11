#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <vector>

namespace levelsync
{
struct LoudnessMeasurement
{
    float db = -120.0f;
    bool reliable = false;
};

// Fast K-weighted estimate, not an integrated or normative short-term LUFS meter.
// Stereo channel powers are summed, never downmixed (opposite phases cannot cancel).
class LoudnessDetector
{
public:
    void prepare(double sampleRate)
    {
        const auto rate = std::isfinite(sampleRate) && sampleRate >= 8000.0
                            ? sampleRate : 48000.0;
        window.assign(static_cast<std::size_t>(std::lround(rate * 0.4)), 0.0);
        smoothing = std::exp(-1.0 / (rate * 0.1));
        presenceSmoothing = std::exp(-1.0 / (rate * 0.01));

        // ITU-R BS.1770-5, Annex 1, Tables 1 and 2 (48 kHz).
        // Re-map the reference filters through the inverse bilinear transform
        // and back at the actual sample rate. No runtime filter allocation.
        for (auto& channel : filters)
        {
            channel[0].design({1.53512485958697, -2.69169618940638, 1.19839281085285},
                              {1.0, -1.69065929318241, 0.73248077421585}, rate);
            channel[1].design({1.0, -2.0, 1.0},
                              {1.0, -1.99004745483398, 0.99007225036621}, rate);
        }
        reset();
    }

    void reset() noexcept
    {
        std::fill(window.begin(), window.end(), 0.0);
        index = filled = invalidCountdown = 0;
        sum = smoothedEnergy = presenceEnergy = 0.0;
        for (auto& channel : filters)
            for (auto& stage : channel)
                stage.reset();
    }

    void push(const std::array<float, 2>& samples, int channels) noexcept
    {
        if (window.empty())
            return;

        double energy = 0.0;
        double rawEnergy = 0.0;
        bool invalid = false;
        for (int ch = 0; ch < std::clamp(channels, 0, 2); ++ch)
        {
            const auto c = static_cast<std::size_t>(ch);
            const double raw = samples[c];
            const bool valid = std::isfinite(raw) && std::abs(raw) <= 1.0e6;
            invalid = invalid || !valid;
            if (!valid)
                for (auto& stage : filters[c])
                    stage.reset();
            const double x = valid ? raw : 0.0;
            rawEnergy += x * x;
            const double weighted = filters[c][1].process(filters[c][0].process(x));
            energy += weighted * weighted;
        }

        invalidCountdown = invalid ? window.size()
                                   : (invalidCountdown > 0 ? invalidCountdown - 1 : 0);
        sum = std::max(0.0, sum + energy - window[index]);
        window[index] = energy;
        index = (index + 1) % window.size();
        filled = std::min(filled + 1, window.size());
        const double mean = sum / static_cast<double>(window.size());
        smoothedEnergy = smoothing * smoothedEnergy + (1.0 - smoothing) * mean;
        presenceEnergy = presenceSmoothing * presenceEnergy
                         + (1.0 - presenceSmoothing) * rawEnergy;
    }

    LoudnessMeasurement measurement() const noexcept
    {
        const auto db = static_cast<float>(-0.691 + 10.0 * std::log10(std::max(1.0e-12, smoothedEnergy)));
        return {std::max(-120.0f, db), !window.empty() && filled == window.size()
                    && invalidCountdown == 0 && db > -70.0f && presenceEnergy > 1.0e-8};
    }

    std::size_t windowSamples() const noexcept { return window.size(); }

private:
    struct Biquad
    {
        std::array<double, 3> b{1.0, 0.0, 0.0};
        std::array<double, 3> a{1.0, 0.0, 0.0};
        double z1 = 0.0, z2 = 0.0;

        void reset() noexcept { z1 = z2 = 0.0; }

        static std::array<double, 3> remap(const std::array<double, 3>& c, double ratio)
        {
            const double p0 = c[0] + c[1] + c[2];
            const double p1 = 2.0 * (c[0] - c[2]) * ratio;
            const double p2 = (c[0] - c[1] + c[2]) * ratio * ratio;
            return {p0 + p1 + p2, 2.0 * (p0 - p2), p0 - p1 + p2};
        }

        void design(const std::array<double, 3>& numerator,
                    const std::array<double, 3>& denominator, double rate)
        {
            b = remap(numerator, rate / 48000.0);
            a = remap(denominator, rate / 48000.0);
            const double normalisation = a[0];
            for (std::size_t i = 0; i < 3; ++i)
            {
                b[i] /= normalisation;
                a[i] /= normalisation;
            }
        }

        double process(double x) noexcept
        {
            const double y = b[0] * x + z1;
            z1 = b[1] * x - a[1] * y + z2;
            z2 = b[2] * x - a[2] * y;
            return y;
        }
    };

    std::array<std::array<Biquad, 2>, 2> filters;
    std::vector<double> window;
    std::size_t index = 0, filled = 0, invalidCountdown = 0;
    double sum = 0.0, smoothedEnergy = 0.0, presenceEnergy = 0.0;
    double smoothing = 0.0, presenceSmoothing = 0.0;
};
}
