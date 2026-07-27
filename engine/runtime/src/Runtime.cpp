#include <papagedon/runtime/Runtime.h>

#include <papagedon/utilities/Logger.h>
#include "../../rendering/DebugState.h"

#include <span>
#include <algorithm>

namespace papagedon::runtime {

Runtime::Runtime(utilities::Logger& logger) noexcept
    : logger_{logger} {}

bool Runtime::Initialize() {
    if (initialized_) {
        return true;
    }
    if (!audioInput_.Load("test.mp3")) {
        logger_.INFO("Failed to load test.mp3. Ensure it is present in the working directory.");
        // We do not fail initialization here, we can run without audio.
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
    // ── 0. Preset input ───────────────────────────────────────────────────────
    // Drain any F1..F6 preset request the renderer latched last frame.  Applying
    // it only swaps an index in the PresetManager — no allocation, no restart.
    const int presetRequest = renderer_.ConsumePresetRequest();
    if (presetRequest >= 0) {
        presetManager_.SetPreset(static_cast<PresetId>(presetRequest));
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
    }

    const audio::ExperienceSignals signals = audioAnalyzer_.Update(audioFrame);

    // ── 3. ExperienceGraph ────────────────────────────────────────────────────
    const ExperienceGraphOutput graphOutput = experienceGraph_.Update(signals);

    // ── 4. SceneDNA ───────────────────────────────────────────────────────────
    sceneDNA_.Update(graphOutput);

    // ── 5. Renderer ───────────────────────────────────────────────────────────
    const SceneState& currentScene = sceneDNA_.GetCurrentScene();
    const ExperiencePreset& activePreset = presetManager_.CurrentPreset();

    DebugState debugState{};
    debugState.bpm                = signals.bpm;
    debugState.energy             = signals.energy;
    debugState.intensity          = signals.intensity;
    debugState.currentExperience  = ToString(graphOutput.event);
    debugState.currentScene       = currentScene.activeProfile
                                        ? currentScene.activeProfile->sceneId.c_str()
                                        : "None";
    debugState.currentPreset      = activePreset.name;
    debugState.transitionProgress = currentScene.transitionProgress;

    renderer_.BeginFrame();
    renderer_.Render(currentScene, debugState, signals, activePreset);
    if (!renderer_.EndFrame()) {
        RequestStop();
    }

    static_cast<void>(deltaTime);
}

} // namespace papagedon::runtime
