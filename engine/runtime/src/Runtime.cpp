#include <papagedon/runtime/Runtime.h>

#include <papagedon/utilities/Logger.h>

namespace papagedon::runtime {

Runtime::Runtime(utilities::Logger& logger) noexcept
    : logger_{logger} {}

bool Runtime::Initialize() {
    if (initialized_) {
        return true;
    }

    audio::AudioInput::Config config{};
    config.source = audio::AudioInput::SourceType::Microphone;
    config.sampleRate = 48'000;
    config.channelCount = 2;
    config.framesPerPeriod = 256;

    if (!audioInput_.Initialize(config)) {
        logger_.ERROR("Failed to initialize audio input.");
        return false;
    }

    if (!audioInput_.Start()) {
        logger_.ERROR("Failed to start audio input.");
        audioInput_.Stop();
        return false;
    }

    initialized_ = true;
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
    audioInput_.Stop();
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
    const audio::AudioFrame latestInput = audioInput_.GetLatestSamples();
    const audio::ExperienceSignals signals = audioAnalyzer_.Update(latestInput);

    static_cast<void>(deltaTime);
    static_cast<void>(signals);
}

} // namespace papagedon::runtime
