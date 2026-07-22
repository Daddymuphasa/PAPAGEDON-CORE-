#include <papagedon/runtime/Runtime.h>

#include <papagedon/utilities/Logger.h>

namespace papagedon::runtime {

Runtime::Runtime(utilities::Logger& logger) noexcept
    : logger_{logger} {}

bool Runtime::Initialize() {
    if (initialized_) {
        return true;
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
    static_cast<void>(deltaTime);
}

} // namespace papagedon::runtime
