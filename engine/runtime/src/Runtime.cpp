#include <papagedon/runtime/Runtime.h>

#include <papagedon/utilities/Logger.h>

#include <cmath>
#include <sstream>

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

    if (!experienceGraph_.Initialize()) {
        logger_.ERROR("Failed to initialize experience graph.");
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
    const papagedon::ExperienceState state = experienceGraph_.Update(signals);

    if (signals.beat && !lastBeatLogged_) {
        std::ostringstream message;
        message << "Beat event detected. BPM=" << signals.bpm
                << ", confidence=" << signals.beatConfidence
                << ", energy=" << signals.energy
                << ", intensity=" << signals.intensity
                << ", tension=" << signals.tension
                << ", bass=" << signals.bassLevel
                << ", mid=" << signals.midLevel
                << ", treble=" << signals.trebleLevel
                << ", loudness=" << signals.loudness
                << ", dynamicRange=" << signals.dynamicRange
                << ", experienceEvent=" << ToString(state.event);
        logger_.INFO(message.str());
        lastBeatLogged_ = true;
    } else if (!signals.beat) {
        lastBeatLogged_ = false;
    }

    if (signals.bpm > 0.0F && std::abs(signals.bpm - lastLoggedBpm_) >= 0.5F) {
        std::ostringstream message;
        message << "Current BPM=" << signals.bpm
                << ", beatConfidence=" << signals.beatConfidence
                << ", energy=" << signals.energy
                << ", intensity=" << signals.intensity
                << ", tension=" << signals.tension
                << ", loudness=" << signals.loudness
                << ", dynamicRange=" << signals.dynamicRange
                << ", experienceEvent=" << ToString(state.event);
        logger_.INFO(message.str());
        lastLoggedBpm_ = signals.bpm;
    }

    if ((debugFrameCounter_ % 30U) == 0U) {
        std::ostringstream message;
        message << "ExperienceState: mood=" << state.mood
                << ", intensity=" << state.intensity
                << ", tension=" << state.tension
                << ", energy=" << state.energy
                << ", transitionProgress=" << state.transitionProgress
                << ", activeScene=" << state.activeScene
                << ", event=" << ToString(state.event);
        logger_.INFO(message.str());
    }

    ++debugFrameCounter_;
    static_cast<void>(deltaTime);
}

} // namespace papagedon::runtime
