#include "Renderer.h"

#include "backends/OpenGLRenderer.h"

namespace papagedon {

Renderer::Renderer()
    : backend_{std::make_unique<OpenGLRenderer>()} {}

Renderer::~Renderer() {
    Shutdown();
}

void Renderer::Configure(bool fullscreen, bool vsync) {
    if (backend_ != nullptr) {
        backend_->Configure(fullscreen, vsync);
    }
}

void Renderer::SetMasterControls(float brightness, float glow, float exposure) {
    if (backend_ != nullptr) {
        backend_->SetMasterControls(brightness, glow, exposure);
    }
}

void Renderer::SetDemoMode(bool enabled) {
    if (backend_ != nullptr) {
        backend_->SetDemoMode(enabled);
    }
}

void Renderer::SetDebugOverlay(bool visible) {
    if (backend_ != nullptr) {
        backend_->SetDebugOverlay(visible);
    }
}

void Renderer::PresentSplash(const std::string& status, float progress) {
    if (backend_ != nullptr) {
        backend_->PresentSplash(status, progress);
    }
}

const char* Renderer::BackendName() const noexcept {
    return backend_ != nullptr ? backend_->BackendName() : "None";
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
    const ExperiencePreset& preset,
    const visual::Theme& theme) {

    if (backend_ != nullptr) {
        backend_->Render(state, debugState, signals, preset, theme);
    }
}

bool Renderer::EndFrame() {
    return backend_ != nullptr && backend_->EndFrame();
}

int Renderer::ConsumeThemeRequest() noexcept {
    return backend_ != nullptr ? backend_->ConsumeThemeRequest() : -1;
}

bool Renderer::ConsumePlayPauseToggle() noexcept {
    return backend_ != nullptr && backend_->ConsumePlayPauseToggle();
}

bool Renderer::ConsumeReloadRequest() noexcept {
    return backend_ != nullptr && backend_->ConsumeReloadRequest();
}

bool Renderer::ConsumeAutoToggle() noexcept {
    return backend_ != nullptr && backend_->ConsumeAutoToggle();
}

void Renderer::Shutdown() noexcept {
    if (backend_ != nullptr) {
        backend_->Shutdown();
    }
}

} // namespace papagedon
