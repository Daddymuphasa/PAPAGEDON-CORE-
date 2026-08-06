#pragma once

#include <atomic>
#include <chrono>
#include <string>
#include <vector>
#include <AudioInput.h>
#include <AudioPlayer.h>
#include <AudioAnalyzer.h>
#include <AudioCapture.h>
#include <ExperienceGraph.h>
#include <AutoDirector.h>
#include <presets/PresetManager.h>
#include <SceneDNA.h>
#include <Renderer.h>
#include <ThemeManager.h>
#include <papagedon/runtime/DemoConfig.h>

namespace papagedon::utilities {
class Logger;
}

namespace papagedon::runtime {

class Runtime final {
public:
    using FrameDuration = std::chrono::duration<double>;

    explicit Runtime(utilities::Logger& logger) noexcept;

    Runtime(const Runtime&) = delete;
    Runtime& operator=(const Runtime&) = delete;

    /// Initializes all systems. An optional audio-file path selects the clip to
    /// play; when empty the default (test.mp3) is used.
    bool Initialize(const std::string& audioPath = {});
    void Run();
    void Shutdown() noexcept;
    void RequestStop() noexcept;

    [[nodiscard]] bool IsRunning() const noexcept;

private:
    void Update(FrameDuration deltaTime) noexcept;

    struct AudioSelection {
        std::string source;      // "input", "loopback", or "file"
        int         deviceIndex = -1;
        bool        isLoopback  = false;
    };
    AudioSelection SelectAudioSource(const std::string& currentSource,
                                     int currentDevice);

    utilities::Logger& logger_;
    audio::AudioInput audioInput_;
    audio::AudioPlayer audioPlayer_;
    audio::AudioCapture audioCapture_;
    audio::AudioAnalyzer audioAnalyzer_;
    ExperienceGraph experienceGraph_;
    PresetManager presetManager_;
    AutoDirector autoDirector_;
    visual::ThemeManager themeManager_;
    SceneDNA sceneDNA_;
    Renderer renderer_;
    std::atomic_bool running_{false};
    bool initialized_ = false;

    // ── Demo Mode ───────────────────────────────────────────────────────────────
    DemoConfig  config_;
    std::string configPath_ = "config/demo.json";
    std::string audioFileName_;      ///< Loaded clip / live device name, for the overlay.
    std::string homeThemeId_ = "badman"; ///< Theme the 'B' key snaps back to (red brand).
    bool        audioReady_ = false; ///< False when audio is unavailable.
    bool        liveAudio_  = false; ///< True when reacting to a live capture device.
    std::vector<float> captureBuffer_; ///< Reused live-audio window (no per-frame alloc).

    // Auto-VJ: when enabled, the AutoDirector chooses presets from the live
    // experience.  Toggled with 'A', or started on with PAPAGEDON_AUTOVJ; any
    // manual F-key press hands control back to the operator.
    bool autoMode_ = false;

    // Optional demo mode: when PAPAGEDON_DEMO_CYCLE is set to a positive number
    // of seconds, the runtime advances to the next preset on that interval.
    // Off by default (zero), so normal runs are unaffected.
    double demoCycleSeconds_ = 0.0;
    double demoCycleElapsed_ = 0.0;
};

} // namespace papagedon::runtime
