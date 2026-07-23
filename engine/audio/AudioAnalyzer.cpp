#include "AudioAnalyzer.h"

#include <algorithm>
#include <cmath>
#include <complex>
#include <cstddef>
#include <deque>
#include <limits>
#include <numeric>
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

[[nodiscard]] float SpectralOnset(
    std::span<const float> currentSpectrum,
    std::span<const float> previousSpectrum) noexcept {
    if (currentSpectrum.empty() || previousSpectrum.empty()) {
        return 0.0F;
    }

    const std::size_t commonSize = std::min(currentSpectrum.size(), previousSpectrum.size());
    float accumulator = 0.0F;
    std::size_t count = 0;
    for (std::size_t index = 0; index < commonSize; ++index) {
        const float delta = std::abs(currentSpectrum[index] - previousSpectrum[index]);
        accumulator += delta;
        ++count;
    }

    return count == 0 ? 0.0F : accumulator / static_cast<float>(count);
}

[[nodiscard]] float EstimateBpm(std::span<const float> intervals) noexcept {
    if (intervals.empty()) {
        return 0.0F;
    }

    std::vector<float> sortedIntervals(intervals.begin(), intervals.end());
    std::sort(sortedIntervals.begin(), sortedIntervals.end());

    const std::size_t middleIndex = sortedIntervals.size() / 2U;
    const float medianInterval = sortedIntervals[middleIndex];
    if (medianInterval <= 0.0F) {
        return 0.0F;
    }

    return 60.0F / medianInterval;
}

[[nodiscard]] float SmoothValue(const float currentValue, const float previousValue, const float smoothing) noexcept {
    return std::clamp(previousValue + (currentValue - previousValue) * smoothing, 0.0F, 1.0F);
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
    previousSpectrum_.clear();
    onsetHistory_.clear();
    beatIntervals_.clear();
    accumulatedSeconds_ = 0.0F;
    lastBeatTimeSeconds_ = -1.0F;
    smoothedBpm_ = 0.0F;
    smoothedEnergy_ = 0.0F;
    smoothedIntensity_ = 0.0F;
    smoothedTension_ = 0.0F;
    smoothedBassLevel_ = 0.0F;
    smoothedMidLevel_ = 0.0F;
    smoothedTrebleLevel_ = 0.0F;
    smoothedLoudness_ = 0.0F;
    smoothedDynamicRange_ = 0.0F;
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

    std::vector<float> padded(kSpectrumBinCount, 0.0F);
    const std::size_t copyCount = std::min<std::size_t>(mono.size(), kSpectrumBinCount);
    std::copy_n(mono.begin(), copyCount, padded.begin());

    const auto transformed = RunFft(padded);
    if (transformed.empty()) {
        return {};
    }

    std::vector<float> spectrum(kSpectrumBinCount);
    const float normalizer = static_cast<float>(kSpectrumBinCount);
    for (std::size_t index = 0; index < transformed.size() && index < spectrum.size(); ++index) {
        const float magnitude = std::abs(transformed[index]) / normalizer;
        spectrum[index] = std::clamp(magnitude, 0.0F, 1.0F);
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
    const float frameDurationSeconds = static_cast<float>(frame.FrameCount()) / static_cast<float>(frame.sampleRate);
    const float bassLevel = std::clamp(bassEnergy, 0.0F, 1.0F);
    const float midLevel = std::clamp(midEnergy, 0.0F, 1.0F);
    const float trebleLevel = std::clamp(trebleEnergy, 0.0F, 1.0F);
    const float loudness = std::clamp((bassLevel * 0.35F) + (midLevel * 0.40F) + (trebleLevel * 0.25F), 0.0F, 1.0F);
    const float dynamicRange = std::clamp(std::max({bassLevel, midLevel, trebleLevel}) - std::min({bassLevel, midLevel, trebleLevel}), 0.0F, 1.0F);

    const float smoothedEnergy = SmoothValue(energy, smoothedEnergy_, kTemporalSmoothing);
    const float smoothedIntensity = SmoothValue(averageBandEnergy * 1.25F, smoothedIntensity_, kTemporalSmoothing);
    const float smoothedTension = SmoothValue(averageBandEnergy * 0.75F, smoothedTension_, kTemporalSmoothing);
    const float smoothedBassLevel = SmoothValue(bassLevel, smoothedBassLevel_, kTemporalSmoothing);
    const float smoothedMidLevel = SmoothValue(midLevel, smoothedMidLevel_, kTemporalSmoothing);
    const float smoothedTrebleLevel = SmoothValue(trebleLevel, smoothedTrebleLevel_, kTemporalSmoothing);
    const float smoothedLoudness = SmoothValue(loudness, smoothedLoudness_, kTemporalSmoothing);
    const float smoothedDynamicRange = SmoothValue(dynamicRange, smoothedDynamicRange_, kTemporalSmoothing);

    accumulatedSeconds_ += frameDurationSeconds;

    const float onset = SpectralOnset(spectrum, previousSpectrum_);
    onsetHistory_.push_back(onset);
    if (onsetHistory_.size() > 16U) {
        onsetHistory_.pop_front();
    }

    const float onsetAverage = std::accumulate(onsetHistory_.begin(), onsetHistory_.end(), 0.0F) /
        static_cast<float>(std::max<std::size_t>(1U, onsetHistory_.size()));
    const float onsetVariance = [&]() {
        float variance = 0.0F;
        for (const float value : onsetHistory_) {
            const float delta = value - onsetAverage;
            variance += delta * delta;
        }
        return onsetHistory_.empty() ? 0.0F : variance / static_cast<float>(onsetHistory_.size());
    }();
    const float onsetDeviation = std::sqrt(onsetVariance);
    const float beatThreshold = std::clamp(onsetAverage + onsetDeviation * 0.75F + 0.02F, 0.02F, 1.0F);
    const float timeSinceLastBeat = (lastBeatTimeSeconds_ >= 0.0F)
        ? accumulatedSeconds_ - lastBeatTimeSeconds_
        : std::numeric_limits<float>::max();

    const bool beatCandidate = onset > beatThreshold && timeSinceLastBeat >= kBeatCooldownSeconds;
    const bool beat = beatCandidate;
    const float beatConfidence = beat
        ? std::clamp((onset - beatThreshold) / std::max(0.15F, 1.0F - beatThreshold), 0.0F, 1.0F)
        : std::clamp(onset / std::max(beatThreshold, 0.05F), 0.0F, 1.0F);

    if (beat) {
        const float intervalSeconds = std::clamp(timeSinceLastBeat, kMinBeatIntervalSeconds, kMaxBeatIntervalSeconds);
        if (lastBeatTimeSeconds_ >= 0.0F) {
            beatIntervals_.push_back(intervalSeconds);
            if (beatIntervals_.size() > 12U) {
                beatIntervals_.pop_front();
            }

            std::vector<float> intervalWindow(beatIntervals_.begin(), beatIntervals_.end());
            const float estimatedBpm = EstimateBpm(intervalWindow);
            const float targetBpm = std::clamp(estimatedBpm, 60.0F, 200.0F);
            smoothedBpm_ = (1.0F - kBpmSmoothing) * smoothedBpm_ + kBpmSmoothing * targetBpm;
        }

        lastBeatTimeSeconds_ = accumulatedSeconds_;
    }

    previousSpectrum_ = spectrum;

    smoothedEnergy_ = smoothedEnergy;
    smoothedIntensity_ = smoothedIntensity;
    smoothedTension_ = smoothedTension;
    smoothedBassLevel_ = smoothedBassLevel;
    smoothedMidLevel_ = smoothedMidLevel;
    smoothedTrebleLevel_ = smoothedTrebleLevel;
    smoothedLoudness_ = smoothedLoudness;
    smoothedDynamicRange_ = smoothedDynamicRange;

    return {
        .energy = smoothedEnergy,
        .intensity = smoothedIntensity,
        .tension = smoothedTension,
        .beat = beat,
        .bpm = std::clamp(smoothedBpm_, 0.0F, 240.0F),
        .beatConfidence = beatConfidence,
        .confidence = std::clamp(
            beatConfidence * 0.5F + smoothedIntensity * 0.5F,
            0.0F,
            1.0F),
        .spectrum = std::move(spectrum),
        .bassEnergy = bassEnergy,
        .midEnergy = midEnergy,
        .trebleEnergy = trebleEnergy,
        .bassLevel = smoothedBassLevel,
        .midLevel = smoothedMidLevel,
        .trebleLevel = smoothedTrebleLevel,
        .loudness = smoothedLoudness,
        .dynamicRange = smoothedDynamicRange,
    };
}

} // namespace papagedon::audio
