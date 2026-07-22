#include "AudioAnalyzer.h"

#include <algorithm>
#include <cmath>
#include <complex>
#include <cstddef>
#include <numbers>
#include <vector>

namespace papagedon::audio {
namespace {

[[nodiscard]] constexpr std::size_t NextPowerOfTwo(std::size_t value) noexcept {
    std::size_t power = 1;
    while (power < value) {
        power <<= 1;
    }
    return power;
}

[[nodiscard]] std::vector<float> ToMono(std::span<const float> samples, std::uint32_t channelCount) noexcept {
    if (channelCount == 0 || samples.empty()) {
        return {};
    }

    const std::size_t frameCount = samples.size() / channelCount;
    std::vector<float> mono(frameCount, 0.0F);

    for (std::size_t frame = 0; frame < frameCount; ++frame) {
        float sum = 0.0F;
        for (std::uint32_t channel = 0; channel < channelCount; ++channel) {
            const auto sampleIndex = frame * channelCount + channel;
            if (sampleIndex >= samples.size()) {
                break;
            }
            sum += samples[sampleIndex];
        }
        mono[frame] = sum / static_cast<float>(channelCount);
    }

    return mono;
}

[[nodiscard]] std::vector<std::complex<float>> RunFft(std::span<const float> samples) noexcept {
    const std::size_t blockSize = NextPowerOfTwo(samples.size());
    if (blockSize < 2) {
        return {};
    }

    std::vector<std::complex<float>> values(blockSize);
    for (std::size_t i = 0; i < samples.size(); ++i) {
        values[i] = std::complex<float>{samples[i], 0.0F};
    }

    for (std::size_t len = 2; len <= blockSize; len <<= 1) {
        const float angle = -2.0F * std::numbers::pi_v<float> / static_cast<float>(len);
        const std::complex<float> twiddle{std::cos(angle), std::sin(angle)};

        for (std::size_t offset = 0; offset < blockSize; offset += len) {
            std::complex<float> factor{1.0F, 0.0F};
            for (std::size_t index = 0; index < len / 2; ++index) {
                const std::complex<float> even = values[offset + index];
                const std::complex<float> odd = values[offset + index + len / 2] * factor;
                values[offset + index] = even + odd;
                values[offset + index + len / 2] = even - odd;
                factor *= twiddle;
            }
        }
    }

    return values;
}

[[nodiscard]] float BandEnergy(
    std::span<const float> spectrum,
    const std::size_t beginBin,
    const std::size_t endBin) noexcept {
    if (beginBin >= spectrum.size() || endBin <= beginBin) {
        return 0.0F;
    }

    const std::size_t clampedEnd = std::min(endBin, spectrum.size());
    float accumulator = 0.0F;
    std::size_t count = 0;
    for (std::size_t index = beginBin; index < clampedEnd; ++index) {
        accumulator += spectrum[index];
        ++count;
    }

    return count == 0 ? 0.0F : accumulator / static_cast<float>(count);
}

} // namespace

ExperienceSignals AudioAnalyzer::Update(const AudioFrame& frame) noexcept {
    latestSignals_ = Analyze(frame);
    return latestSignals_;
}

ExperienceSignals AudioAnalyzer::Update(
    const std::span<const float> samples,
    const std::uint32_t sampleRate,
    const std::uint32_t channelCount) noexcept {
    return Update(AudioFrame{samples, sampleRate, channelCount});
}

const ExperienceSignals& AudioAnalyzer::LatestSignals() const noexcept {
    return latestSignals_;
}

void AudioAnalyzer::Reset() noexcept {
    latestSignals_ = {};
}

std::vector<float> AudioAnalyzer::ComputeSpectrum(
    const std::span<const float> samples,
    const std::uint32_t sampleRate,
    const std::uint32_t channelCount) noexcept {
    if (samples.empty() || sampleRate == 0 || channelCount == 0) {
        return {};
    }

    const auto mono = ToMono(samples, channelCount);
    if (mono.empty()) {
        return {};
    }

    const auto transformed = RunFft(mono);
    if (transformed.empty()) {
        return {};
    }

    std::vector<float> spectrum(transformed.size() / 2U);
    const float normalizer = static_cast<float>(mono.size());
    const float nyquist = static_cast<float>(sampleRate) / 2.0F;
    for (std::size_t index = 0; index < spectrum.size(); ++index) {
        const float magnitude = std::abs(transformed[index]) / normalizer;
        spectrum[index] = std::clamp(magnitude, 0.0F, 1.0F);
    }

    if (nyquist <= 0.0F) {
        return spectrum;
    }

    return spectrum;
}

ExperienceSignals AudioAnalyzer::Analyze(const AudioFrame& frame) const noexcept {
    if (!frame.IsValid() || frame.samples.empty()) {
        return {};
    }

    float totalAmplitude = 0.0F;
    for (const float sample : frame.samples) {
        if (!std::isfinite(sample)) {
            return {};
        }

        totalAmplitude += std::clamp(std::abs(sample), 0.0F, 1.0F);
    }

    const float energy = totalAmplitude / static_cast<float>(frame.samples.size());
    auto spectrum = ComputeSpectrum(frame.samples, frame.sampleRate, frame.channelCount);
    const float bassEnergy = BandEnergy(spectrum, 0, std::max<std::size_t>(1, spectrum.size() / 4U));
    const float midEnergy = BandEnergy(spectrum, spectrum.size() / 4U, spectrum.size() / 2U);
    const float trebleEnergy = BandEnergy(spectrum, spectrum.size() / 2U, spectrum.size());

    const float averageBandEnergy = (bassEnergy + midEnergy + trebleEnergy) / 3.0F;

    return {
        .energy = energy,
        .intensity = std::clamp(averageBandEnergy * 1.25F, 0.0F, 1.0F),
        .tension = std::clamp(averageBandEnergy * 0.75F, 0.0F, 1.0F),
        .beat = false,
        .bpm = 0.0F,
        .confidence = std::min(
            1.0F,
            static_cast<float>(frame.samples.size()) / 1'024.0F),
        .spectrum = std::move(spectrum),
        .bassEnergy = bassEnergy,
        .midEnergy = midEnergy,
        .trebleEnergy = trebleEnergy,
    };
}

} // namespace papagedon::audio
