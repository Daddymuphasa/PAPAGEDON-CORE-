#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include "InputLatency.h"

namespace papagedon::audio {

// ──────────────────────────────────────────────────────────────────────────────
// AudioCapture
//
// Live audio input for a real set: opens a capture device (a DJ mixer / audio
// interface feeding the machine) or a system loopback, and keeps the most recent
// audio in a ring buffer. The analysis pipeline reads the latest window each
// frame, so PAPAGEDON reacts to whatever is actually playing in the booth.
//
// Capture runs on the audio thread; ReadLatest() is called from the main thread.
// The ring is single-producer / single-consumer — a rare torn sample at the
// boundary is irrelevant to visualisation.
// ──────────────────────────────────────────────────────────────────────────────
class AudioCapture final {
public:
    AudioCapture();
    ~AudioCapture();

    AudioCapture(const AudioCapture&) = delete;
    AudioCapture& operator=(const AudioCapture&) = delete;

    /// Opens a device.  `deviceIndex` -1 selects the default; `loopback` captures
    /// the system's default output instead of an input.  Returns false on failure.
    bool Initialize(int deviceIndex = -1, bool loopback = false);

    bool Start();
    void Stop();
    void Shutdown() noexcept;

    [[nodiscard]] bool IsOpen() const noexcept;
    [[nodiscard]] std::uint32_t SampleRate() const noexcept;
    [[nodiscard]] std::uint32_t Channels() const noexcept;
    [[nodiscard]] const std::string& DeviceName() const noexcept;

    /// Copies the most recent `frames` interleaved samples into `out` (resized to
    /// frames * channels, zero-padded if fewer are available).  Returns the number
    /// of frames actually available.
    std::size_t ReadLatest(std::vector<float>& out, std::size_t frames) const;

    /// Monotonic audio-domain time (seconds) of the most recent captured sample.
    [[nodiscard]] double CapturedSeconds() const noexcept;
    [[nodiscard]] CaptureTiming Timing() const noexcept;

    /// Prints the available capture and playback (loopback) devices to stdout.
    static void ListDevices();

    /// Structured device info returned by EnumerateDevices().
    struct DeviceInfo {
        std::string name;
        int         index     = -1;
        bool        isDefault = false;
        bool        isCapture = true;  ///< true = input/capture, false = output/loopback.
    };

    /// Returns all available audio devices (capture + playback) without printing.
    [[nodiscard]] static std::vector<DeviceInfo> EnumerateDevices();

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace papagedon::audio
