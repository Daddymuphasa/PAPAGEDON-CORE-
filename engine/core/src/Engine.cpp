#include <papagedon/core/Engine.h>

namespace papagedon::core {

Engine::Engine()
    : runtime_{logger_} {}

Engine::~Engine() {
    Shutdown();
}

bool Engine::Initialize() {
    if (initialized_) {
        return true;
    }

    initialized_ = runtime_.Initialize();
    if (initialized_) {
        logger_.INFO("Engine initialized.");
    }

    return initialized_;
}

void Engine::Run() {
    if (!initialized_) {
        logger_.ERROR("Engine must be initialized before it can run.");
        return;
    }

    runtime_.Run();
}

void Engine::Shutdown() noexcept {
    if (!initialized_) {
        return;
    }

    runtime_.Shutdown();
    initialized_ = false;
    logger_.INFO("Engine shut down.");
}

void Engine::RequestStop() noexcept {
    runtime_.RequestStop();
}

} // namespace papagedon::core
