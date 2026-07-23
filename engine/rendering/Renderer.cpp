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

void Renderer::Render(const SceneState& sceneState) {
    if (backend_ != nullptr) {
        backend_->Render(sceneState);
    }
}

void Renderer::EndFrame() {
    if (backend_ != nullptr) {
        backend_->EndFrame();
    }
}

void Renderer::Shutdown() noexcept {
    if (backend_ != nullptr) {
        backend_->Shutdown();
    }
}

} // namespace papagedon
