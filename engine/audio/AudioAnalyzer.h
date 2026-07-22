#pragma once

#include "AudioFrame.h"
#include "ExperienceSignals.h"

#include <cstdint>
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
    [[nodiscard]] ExperienceSignals Analyze(const AudioFrame& frame) const noexcept;
    [[nodiscard]] static std::vector<float> ComputeSpectrum(
        std::span<const float> samples,
        std::uint32_t sampleRate,
        std::uint32_t channelCount) noexcept;

    ExperienceSignals latestSignals_{};
};

} // namespace papagedon::audio
