#include <papagedon/runtime/Runtime.h>

#include <papagedon/utilities/Logger.h>
#include "../../rendering/DebugState.h"

#include <span>
#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <string>

namespace papagedon::runtime {

Runtime::Runtime(utilities::Logger& logger) noexcept
    : logger_{logger} {}

bool Runtime::Initialize(const std::string& audioPath) {
    if (initialized_) {
        return true;
    }
    const std::string path = audioPath.empty() ? std::string{"test.mp3"} : audioPath;
    if (!audioInput_.Load(path)) {
        logger_.INFO("Failed to load audio file '" + path +
                     "'. Ensure it exists in the working directory.");
        // We do not fail initialization here, we can run without audio.
    } else {
        logger_.INFO("Loaded audio file '" + path + "'.");
    }

    if (!audioPlayer_.Initialize()) {
        logger_.ERROR("AudioPlayer failed to initialize.");
        return false;
    }
    audioPlayer_.Load(&audioInput_);

    initialized_ = sceneDNA_.Initialize();
    if (!initialized_) {
        logger_.ERROR("Scene DNA failed to initialize.");
        return false;
    }
    initialized_ = renderer_.Initialize();
    if (!initialized_) {
        sceneDNA_.Shutdown();
        logger_.ERROR("Renderer failed to initialize.");
        return false;
    }
    if (const char* const cycle = std::getenv("PAPAGEDON_DEMO_CYCLE")) {
        demoCycleSeconds_ = std::atof(cycle);
        if (demoCycleSeconds_ > 0.0) {
            logger_.INFO("Demo preset auto-cycle enabled.");
        }
    }

    if (std::getenv("PAPAGEDON_AUTOVJ") != nullptr) {
        autoMode_ = true;
        logger_.INFO("Auto-VJ enabled at startup.");
    }

    // Load theme JSON files so they can be edited and hot-reloaded (R key)
    // without a rebuild.  A missing directory is fine — the seven built-in themes
    // remain the fallback.  Override the location with PAPAGEDON_THEME_DIR.
    {
        const char* const dirEnv = std::getenv("PAPAGEDON_THEME_DIR");
        const std::string themeDir = (dirEnv != nullptr) ? std::string(dirEnv)
                                                         : std::string("themes");
        if (const std::size_t loaded = themeManager_.LoadThemesFromDirectory(themeDir);
            loaded > 0) {
            logger_.INFO("Loaded " + std::to_string(loaded) + " theme file(s) from '" +
                         themeDir + "'.");
        }
    }

    // Optional theme selection. PAPAGEDON_THEME_FILE loads a theme from JSON on
    // disk (and exercises ReloadTheme); PAPAGEDON_THEME selects a built-in by id.
    if (const char* const themeFile = std::getenv("PAPAGEDON_THEME_FILE")) {
        std::string error;
        if (themeManager_.LoadTheme(themeFile, &error)) {
            logger_.INFO("Loaded theme '" + themeManager_.CurrentTheme().name +
                         "' from '" + std::string(themeFile) + "'.");
        } else {
            logger_.INFO("Failed to load theme file: " + error);
        }
    } else if (const char* const themeId = std::getenv("PAPAGEDON_THEME")) {
        if (themeManager_.SetTheme(themeId)) {
            logger_.INFO("Active theme: " + themeManager_.CurrentTheme().name + ".");
        } else {
            logger_.INFO("Unknown theme id '" + std::string(themeId) +
                         "'; keeping default (" + themeManager_.CurrentTheme().name + ").");
        }
    }

    logger_.INFO("Runtime initialized.");
    return true;
}

void Runtime::Run() {
    if (!initialized_) {
        logger_.ERROR("Runtime must be initialized before it can run.");
        return;
    }

    running_.store(true, std::memory_order_release);
    auto previousFrameTime = std::chrono::steady_clock::now();
    logger_.INFO("Runtime loop started.");

    audioPlayer_.Play();

    while (running_.load(std::memory_order_acquire)) {
        const auto currentFrameTime = std::chrono::steady_clock::now();
        const FrameDuration deltaTime = currentFrameTime - previousFrameTime;
        previousFrameTime = currentFrameTime;

        Update(deltaTime);
    }

    logger_.INFO("Runtime loop stopped.");
}

void Runtime::Shutdown() noexcept {
    if (!initialized_) {
        return;
    }
    RequestStop();
    audioPlayer_.Shutdown();
    audioInput_.Close();
    renderer_.Shutdown();
    sceneDNA_.Shutdown();
    initialized_ = false;
    logger_.INFO("Runtime shut down.");
}

void Runtime::RequestStop() noexcept {
    running_.store(false, std::memory_order_release);
}

bool Runtime::IsRunning() const noexcept {
    return running_.load(std::memory_order_acquire);
}

// ──────────────────────────────────────────────────────────────────────────────
// Runtime::Update
//
// Drives the full Stage 5.0 pipeline each frame in order:
//
//   AudioInput (placeholder)
//       ↓
//   AudioAnalyzer  →  ExperienceSignals
//       ↓
//   ExperienceGraph  →  ExperienceGraphOutput
//       ↓
//   SceneDNA  →  SceneState
//       ↓
//   Renderer
//
// deltaTime is available for future frame-rate-independent interpolation.
// ──────────────────────────────────────────────────────────────────────────────
void Runtime::Update(const FrameDuration deltaTime) noexcept {
    // ── 0. Live controls (Task 7) ──────────────────────────────────────────────
    // Space: play / pause the audio (the AudioPlayer owns the device).
    if (renderer_.ConsumePlayPauseToggle()) {
        audioPlayer_.TogglePlayPause();
        logger_.INFO(audioPlayer_.IsPlaying() ? "Playback resumed." : "Playback paused.");
    }

    // F1..F7: switch theme instantly — same music, a new visual identity.
    if (const int themeSlot = renderer_.ConsumeThemeRequest(); themeSlot >= 0) {
        if (themeManager_.SetThemeByIndex(static_cast<std::size_t>(themeSlot))) {
            logger_.INFO("Theme: " + themeManager_.CurrentTheme().name + ".");
        }
    }

    // R: reload the current theme's JSON from disk, live.
    if (renderer_.ConsumeReloadRequest()) {
        std::string reloadError;
        if (themeManager_.ReloadTheme(&reloadError)) {
            logger_.INFO("Reloaded theme '" + themeManager_.CurrentTheme().name + "'.");
        } else {
            logger_.INFO("Theme reload skipped: " + reloadError);
        }
    }

    // A: toggle Auto-VJ (automatic preset/form selection).
    if (renderer_.ConsumeAutoToggle()) {
        autoMode_ = !autoMode_;
        logger_.INFO(autoMode_ ? "Auto-VJ enabled." : "Auto-VJ disabled.");
    }

    // Optional demo auto-cycle: step presets on a fixed interval (off in Auto-VJ).
    if (!autoMode_ && demoCycleSeconds_ > 0.0) {
        demoCycleElapsed_ += deltaTime.count();
        if (demoCycleElapsed_ >= demoCycleSeconds_) {
            presetManager_.NextPreset();
            demoCycleElapsed_ = 0.0;
        }
    }

    // ── 1. AudioPlayer ────────────────────────────────────────────────────────
    audioPlayer_.Update();

    // ── 2. AudioInput & AudioAnalyzer ─────────────────────────────────────────
    audio::AudioFrame audioFrame{};
    
    const uint64_t currentFrame = audioPlayer_.GetPlaybackPositionInFrames();
    const uint32_t channels = audioInput_.Channels();
    const uint32_t sampleRate = audioInput_.SampleRate();
    const uint64_t totalFrames = audioInput_.FrameCount();
    
    if (sampleRate > 0 && totalFrames > 0 && currentFrame < totalFrames) {
        // Read up to 1024 frames starting from the current playback position
        const uint64_t framesToRead = std::min<uint64_t>(1024, totalFrames - currentFrame);
        const std::span<const float> samples = audioInput_.GetSamples();
        const float* src = samples.data() + (currentFrame * channels);
        
        audioFrame.samples = std::span<const float>(src, framesToRead * channels);
        audioFrame.sampleRate = sampleRate;
        audioFrame.channelCount = channels;
        // Audio-clock timestamp of this window; keeps tempo/beat timing
        // independent of the render frame rate.
        audioFrame.timestampSeconds =
            static_cast<double>(currentFrame) / static_cast<double>(sampleRate);
    }

    const audio::ExperienceSignals signals = audioAnalyzer_.Update(audioFrame);

    // ── 3. ExperienceGraph ────────────────────────────────────────────────────
    const ExperienceGraphOutput graphOutput = experienceGraph_.Update(signals);

    // ── 3.5 Auto-VJ ───────────────────────────────────────────────────────────
    // When enabled, the director picks the preset from the live experience.
    if (autoMode_) {
        presetManager_.SetPreset(
            autoDirector_.Update(graphOutput, static_cast<float>(deltaTime.count())));
    }

    // ── 4. SceneDNA ───────────────────────────────────────────────────────────
    sceneDNA_.Update(graphOutput);

    // ── 5. Renderer ───────────────────────────────────────────────────────────
    const SceneState& currentScene = sceneDNA_.GetCurrentScene();
    const ExperiencePreset& activePreset = presetManager_.CurrentPreset();
    const visual::Theme& activeTheme = themeManager_.CurrentTheme();

    DebugState debugState{};
    debugState.bpm                = signals.bpm;
    debugState.energy             = signals.energy;
    debugState.intensity          = signals.intensity;
    debugState.bass               = signals.bass;
    debugState.mid                = signals.mid;
    debugState.treble             = signals.treble;
    debugState.beat               = signals.beat;
    debugState.currentExperience  = ToString(graphOutput.state);
    debugState.currentScene       = currentScene.activeProfile
                                        ? currentScene.activeProfile->sceneId.c_str()
                                        : "None";
    debugState.currentPreset      = activePreset.name;
    debugState.currentTheme       = activeTheme.name.c_str();
    debugState.autoMode           = autoMode_;
    debugState.transitionProgress = currentScene.transitionProgress;

    renderer_.BeginFrame();
    renderer_.Render(currentScene, debugState, signals, activePreset, activeTheme);
    if (!renderer_.EndFrame()) {
        RequestStop();
    }
}

} // namespace papagedon::runtime
