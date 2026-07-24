#pragma once

#include "../IRenderer.h"

#include <memory>

namespace papagedon {

/// GLFW/GLAD-backed renderer. Its graphics API details remain private to the backend.
class OpenGLRenderer final : public IRenderer {
public:
    explicit OpenGLRenderer(bool vsyncEnabled = true);
    ~OpenGLRenderer() override;

    OpenGLRenderer(const OpenGLRenderer&) = delete;
    OpenGLRenderer& operator=(const OpenGLRenderer&) = delete;

    bool Initialize() override;
    void BeginFrame() override;
    void Render(const SceneState& state) override;
    [[nodiscard]] bool EndFrame() override;
    void Shutdown() noexcept override;

private:
    class Implementation;

    std::unique_ptr<Implementation> implementation_;
    bool vsyncEnabled_ = true;
    bool initialized_ = false;
};

} // namespace papagedon
