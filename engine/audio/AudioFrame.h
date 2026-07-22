#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

namespace papagedon::audio {

/// A non-owning view of one interleaved PCM audio frame.
///
/// The caller retains ownership of samples and must keep them alive for the
/// duration of a call to AudioAnalyzer::Update().
struct AudioFrame final {
    std::span<const float> samples{};
    std::uint32_t sampleRate = 0;
    std::uint32_t channelCount = 0;

    [[nodiscard]] bool IsValid() const noexcept;
    [[nodiscard]] std::size_t FrameCount() const noexcept;
};

} // namespace papagedon::audio
