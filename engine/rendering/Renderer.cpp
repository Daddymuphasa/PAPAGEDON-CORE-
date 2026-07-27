#include "Renderer.h"

#include "backends/OpenGLRenderer.h"

namespace papagedon {

Renderer::Renderer()
    : backend_{std::make_unique<OpenGLRenderer>()} {}

Renderer::~Renderer() {
    Shutdown();
}

bool Renderer::Initialize() {
    return backend_ != nullptr && backend_->Initialize();
}

void Renderer::BeginFrame() {
    if (backend_ != nullptr) {
        backend_->BeginFrame();
    }
}

void Renderer::Render(
    const SceneState&     state,
    const DebugState&     debugState,
    const audio::ExperienceSignals& signals,
    const ExperiencePreset& preset) {

    if (backend_ != nullptr) {
        backend_->Render(state, debugState, signals, preset);
    }
}

bool Renderer::EndFrame() {
    return backend_ != nullptr && backend_->EndFrame();
}

int Renderer::ConsumePresetRequest() noexcept {
    return backend_ != nullptr ? backend_->ConsumePresetRequest() : -1;
}

void Renderer::Shutdown() noexcept {
    if (backend_ != nullptr) {
        backend_->Shutdown();
    }
}

} // namespace papagedon
