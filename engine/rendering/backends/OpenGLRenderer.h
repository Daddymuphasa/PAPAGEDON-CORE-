#pragma once

#include "../IRenderer.h"

namespace papagedon {

/// Placeholder OpenGL backend. Graphics initialization is intentionally deferred.
class OpenGLRenderer final : public IRenderer {
public:
    bool Initialize() override;
    void BeginFrame() override;
    void Render(const SceneState& sceneState) override;
    void EndFrame() override;
    void Shutdown() noexcept override;

private:
    bool initialized_ = false;
};

} // namespace papagedon
