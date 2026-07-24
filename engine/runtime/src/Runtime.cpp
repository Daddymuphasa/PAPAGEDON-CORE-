#include <papagedon/runtime/Runtime.h>

#include <papagedon/utilities/Logger.h>
#include "../../rendering/DebugState.h"

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

void Runtime::Update(const FrameDuration deltaTime) noexcept {
    const audio::AudioFrame placeholderInput{
        .sampleRate = 48'000,
        .channelCount = 2,
    };
    const audio::ExperienceSignals signals = audioAnalyzer_.Update(placeholderInput);

    const ExperienceState experienceState = ExperienceState::Ambient;
    sceneDNA_.Update(experienceState);

    const auto& currentScene = sceneDNA_.GetCurrentScene();

    DebugState debugState{};
    debugState.bpm = signals.bpm;
    debugState.energy = signals.energy;
    debugState.intensity = signals.intensity;
    debugState.currentExperience = ToString(experienceState);
    debugState.transitionProgress = currentScene.transitionProgress;

    renderer_.BeginFrame();
    renderer_.Render(currentScene, debugState);
    if (!renderer_.EndFrame()) {
        RequestStop();
    }

    static_cast<void>(deltaTime);
}

} // namespace papagedon::runtime
