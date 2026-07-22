#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace aei {

/// Decodes an audio file into interleaved 32-bit floating-point PCM samples.
///
/// This class performs no playback or signal analysis.  A failed Load() leaves
/// the object empty, just like Close().
class AudioInput final {
public:
    AudioInput() = default;
    ~AudioInput() = default;

    AudioInput(const AudioInput&) = delete;
    AudioInput& operator=(const AudioInput&) = delete;
    AudioInput(AudioInput&&) noexcept = default;
    AudioInput& operator=(AudioInput&&) noexcept = default;

    /// Decodes path into memory. Returns false when the file cannot be decoded.
    [[nodiscard]] bool Load(const std::string& path);

    /// Releases all decoded PCM data and resets the metadata.
    void Close() noexcept;

    [[nodiscard]] std::uint32_t SampleRate() const noexcept;
    [[nodiscard]] std::uint32_t Channels() const noexcept;
    [[nodiscard]] std::uint64_t FrameCount() const noexcept;

private:
    // Samples are interleaved and use the normalized [-1.0f, 1.0f] format
    // requested from miniaudio during decoding.
    std::vector<float> pcmData_;
    std::uint32_t sampleRate_ = 0;
    std::uint32_t channels_ = 0;
    std::uint64_t frameCount_ = 0;
};

} // namespace aei
