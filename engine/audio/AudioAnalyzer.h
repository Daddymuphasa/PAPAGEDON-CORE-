#pragma once

#include "AudioFrame.h"
#include "ExperienceSignals.h"
#include "SimpleFFT.h"

#include <cstdint>
#include <span>
#include <vector>
#include <memory>

namespace papagedon::audio {

/// Converts PCM sample frames into high-level experience signals using FFT.
class AudioAnalyzer final {
public:
    AudioAnalyzer();
    ~AudioAnalyzer();

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
    [[nodiscard]] ExperienceSignals Analyze(const AudioFrame& frame) noexcept;

    /// Fold one plausible inter-beat interval into the running tempo estimate.
    void UpdateTempo(double intervalSeconds) noexcept;

    ExperienceSignals latestSignals_{};

    // FFT state
    std::unique_ptr<SimpleFFT> fft_;
    std::vector<float> monoBuffer_;
    std::vector<std::complex<float>> fftOutput_;

    // Beat detection state (adaptive bass-energy threshold + onset flux)
    float bassHistorySum_ = 0.0f;
    std::vector<float> bassHistory_;
    size_t bassHistoryIndex_ = 0;
    float previousBass_ = 0.0f;

    // Tempo tracking — timed from the audio playback clock, so it is
    // independent of how fast the render loop calls the analyzer.
    double lastBeatTimeSeconds_ = -1.0;
    std::vector<float> beatBpmHistory_;
    size_t beatBpmIndex_ = 0;
    float currentBpm_ = 0.0f;
};

} // namespace papagedon::audio
