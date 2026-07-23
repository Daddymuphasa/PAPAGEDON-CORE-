#pragma once

#include "AudioFrame.h"
#include "ExperienceSignals.h"

#include <cstdint>
#include <deque>
#include <span>

namespace papagedon::audio {

/// Converts PCM sample frames into high-level experience signals.
///
/// This foundation intentionally uses only lightweight placeholder analysis.
/// Decoding, playback, and spectral analysis remain outside this interface.
class AudioAnalyzer final {
public:
    AudioAnalyzer() = default;

    AudioAnalyzer(const AudioAnalyzer&) = delete;
    AudioAnalyzer& operator=(const AudioAnalyzer&) = delete;

    /// Analyzes one non-owning frame and returns the resulting signals.
    [[nodiscard]] ExperienceSignals Update(const AudioFrame& frame) noexcept;

    /// Convenience overload for callers that have raw interleaved samples.
    [[nodiscard]] ExperienceSignals Update(
        std::span<const float> samples,
        std::uint32_t sampleRate,
        std::uint32_t channelCount) noexcept;

    [[nodiscard]] const ExperienceSignals& LatestSignals() const noexcept;
    void Reset() noexcept;

private:
    static constexpr std::size_t kSpectrumBinCount = 512;
    static constexpr float kMinBeatIntervalSeconds = 0.20F;
    static constexpr float kMaxBeatIntervalSeconds = 2.50F;
    static constexpr float kBeatCooldownSeconds = 0.35F;
    static constexpr float kBpmSmoothing = 0.18F;
    static constexpr float kTemporalSmoothing = 0.30F;

    [[nodiscard]] ExperienceSignals Analyze(const AudioFrame& frame) const noexcept;
    [[nodiscard]] static std::vector<float> ComputeSpectrum(
        std::span<const float> samples,
        std::uint32_t sampleRate,
        std::uint32_t channelCount) noexcept;

    mutable ExperienceSignals latestSignals_{};
    mutable std::vector<float> previousSpectrum_{};
    mutable std::deque<float> onsetHistory_{};
    mutable std::deque<float> beatIntervals_{};
    mutable float accumulatedSeconds_ = 0.0F;
    mutable float lastBeatTimeSeconds_ = -1.0F;
    mutable float smoothedBpm_ = 0.0F;
    mutable float smoothedEnergy_ = 0.0F;
    mutable float smoothedIntensity_ = 0.0F;
    mutable float smoothedTension_ = 0.0F;
    mutable float smoothedBassLevel_ = 0.0F;
    mutable float smoothedMidLevel_ = 0.0F;
    mutable float smoothedTrebleLevel_ = 0.0F;
    mutable float smoothedLoudness_ = 0.0F;
    mutable float smoothedDynamicRange_ = 0.0F;
};

} // namespace papagedon::audio
