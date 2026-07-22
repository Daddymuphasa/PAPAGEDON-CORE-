#pragma once

#include "AudioFrame.h"

#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

#include <miniaudio.h>

struct ma_device;
struct ma_decoder;

namespace papagedon::audio {

/// Captures raw PCM audio from a microphone or a file-backed source.
///
/// This layer is intentionally separate from analysis, runtime ownership, and
/// rendering. It only exposes the latest raw PCM data for consumption by
/// AudioAnalyzer and future audio-intelligence stages.
class AudioInput final {
public:
    enum class SourceType {
        Microphone,
        File,
    };

    struct Config final {
        SourceType source = SourceType::Microphone;
        std::string filePath{};
        std::uint32_t sampleRate = 48'000;
        std::uint32_t channelCount = 2;
        std::uint32_t framesPerPeriod = 256;
    };

    AudioInput() = default;

    AudioInput(const AudioInput&) = delete;
    AudioInput& operator=(const AudioInput&) = delete;
    AudioInput(AudioInput&&) noexcept = delete;
    AudioInput& operator=(AudioInput&&) noexcept = delete;

    /// Initializes the configured input source. The default configuration uses
    /// the system microphone with a 48 kHz stereo capture path.
    [[nodiscard]] bool Initialize();

    /// Initializes the input layer with an explicit source configuration.
    [[nodiscard]] bool Initialize(const Config& config);

    /// Starts the configured capture source.
    [[nodiscard]] bool Start();

    /// Stops the current capture source and clears ownership state.
    void Stop() noexcept;

    /// Returns the latest PCM samples as a non-owning frame view.
    [[nodiscard]] AudioFrame GetLatestSamples() const noexcept;

    [[nodiscard]] bool IsRunning() const noexcept;

private:
    bool InitializeMicrophone();
    bool InitializeFileSource();
    void ResetState() noexcept;

    static void CaptureCallback(
        ma_device* device,
        void* output,
        const void* input,
        ma_uint32 frameCount);

    Config config_{};
    mutable std::mutex mutex_{};
    std::vector<float> latestSamples_{};
    std::uint32_t sampleRate_ = 0;
    std::uint32_t channelCount_ = 0;
    std::uint64_t frameCount_ = 0;
    std::atomic_bool running_{false};
    bool initialized_ = false;
    bool deviceInitialized_ = false;
    ma_device* device_ = nullptr;
    ma_decoder* decoder_ = nullptr;
};

} // namespace papagedon::audio
