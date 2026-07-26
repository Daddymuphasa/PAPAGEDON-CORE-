#include <papagedon/runtime/Runtime.h>

#include <papagedon/utilities/Logger.h>
#include "../../rendering/DebugState.h"
#include "../../rendering/ShaderUniforms.h"

namespace papagedon::runtime {

Runtime::Runtime(utilities::Logger& logger) noexcept
    : logger_{logger} {}

bool Runtime::Initialize() {
    if (initialized_) {
        return true;
    }

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
    // ── 1. Audio Input ────────────────────────────────────────────────────────
    // Placeholder input until the real AudioInput subsystem is wired.
    const audio::AudioFrame audioInput{
        .sampleRate   = 48'000,
        .channelCount = 2,
    };

    // ── 2. AudioAnalyzer ──────────────────────────────────────────────────────
    const audio::ExperienceSignals signals = audioAnalyzer_.Update(audioInput);

    // ── 3. ExperienceGraph ────────────────────────────────────────────────────
    // Consumes ExperienceSignals, resolves event, state, intensity, mood, energy.
    const ExperienceGraphOutput graphOutput = experienceGraph_.Update(signals);

    // ── 4. SceneDNA ───────────────────────────────────────────────────────────
    // Maps ExperienceState to a SceneProfile and forwards visual parameters.
    sceneDNA_.Update(graphOutput);

    // ── 5. Renderer ───────────────────────────────────────────────────────────
    // Consumes SceneState only — contains no audio or experience types.
    const SceneState& currentScene = sceneDNA_.GetCurrentScene();

    DebugState debugState{};
    debugState.bpm                = signals.bpm;
    debugState.energy             = signals.energy;
    debugState.intensity          = signals.intensity;
    debugState.currentExperience  = ToString(graphOutput.event);
    debugState.currentScene       = currentScene.activeProfile
                                        ? currentScene.activeProfile->sceneId.c_str()
                                        : "None";
    debugState.transitionProgress = currentScene.transitionProgress;

    ShaderUniforms uniforms{
        .energy    = currentScene.energy,
        .intensity = currentScene.intensity,
        .bass      = signals.energy,
        .mid       = signals.intensity,
        .treble    = signals.tension,
        .beat      = signals.beat ? 1.0F : 0.0F,
    };

    renderer_.BeginFrame();
    renderer_.Render(currentScene, debugState, uniforms);
    if (!renderer_.EndFrame()) {
        RequestStop();
    }

    static_cast<void>(deltaTime);
}

} // namespace papagedon::runtime
