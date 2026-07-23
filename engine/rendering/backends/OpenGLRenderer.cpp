#include "OpenGLRenderer.h"

namespace papagedon {

bool OpenGLRenderer::Initialize() {
    initialized_ = true;
    return true;
}

void OpenGLRenderer::BeginFrame() {
    // OpenGL frame setup will be introduced by the graphics implementation.
}

void OpenGLRenderer::Render(const SceneState& sceneState) {
    static_cast<void>(sceneState);
    // Rendering is intentionally deferred.
}

void OpenGLRenderer::EndFrame() {
    // Presentation is intentionally deferred.
}

void OpenGLRenderer::Shutdown() noexcept {
    initialized_ = false;
}

} // namespace papagedon
