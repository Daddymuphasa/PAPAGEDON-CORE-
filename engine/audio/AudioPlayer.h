#pragma once

#include <cstdint>

namespace papagedon::audio {

class AudioInput;

/// Dedicated subsystem for audio playback.
/// Responsible for device initialization, playback state, and timing.
/// Does not decode or analyze audio.
class AudioPlayer final {
public:
    AudioPlayer();
    ~AudioPlayer();

    AudioPlayer(const AudioPlayer&) = delete;
    AudioPlayer& operator=(const AudioPlayer&) = delete;
    AudioPlayer(AudioPlayer&&) noexcept = default;
    AudioPlayer& operator=(AudioPlayer&&) noexcept = default;

    /// Initializes the audio device.
    bool Initialize();

    /// Binds the decoded AudioInput to be played.
    void Load(const AudioInput* input);

    void Play();
    void Pause();
    void Stop();

    /// Toggles between playing and paused.
    void TogglePlayPause();

    /// True while audio is actively being played out.
    [[nodiscard]] bool IsPlaying() const noexcept;

    /// Seeks to a specific frame.
    void Seek(std::uint64_t frameIndex);

    /// Must be called every frame by the runtime.
    void Update();

    /// Returns the current playback position in PCM frames.
    [[nodiscard]] std::uint64_t GetPlaybackPositionInFrames() const noexcept;

    /// Returns the current playback time in seconds.
    [[nodiscard]] double GetCurrentPlaybackTime() const noexcept;

    void Shutdown() noexcept;

private:
    class Implementation;
    Implementation* impl_ = nullptr;
};

} // namespace papagedon::audio
