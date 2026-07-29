#pragma once

#include "../IRenderer.h"
#include "../ShaderManager.h"

#include <memory>

namespace papagedon {

/// GLFW/GLAD-backed renderer. All GL details remain private to this backend.
class OpenGLRenderer final : public IRenderer {
public:
    explicit OpenGLRenderer(bool vsyncEnabled = true);
    ~OpenGLRenderer() override;

    OpenGLRenderer(const OpenGLRenderer&) = delete;
    OpenGLRenderer& operator=(const OpenGLRenderer&) = delete;

    bool Initialize() override;
    void BeginFrame() override;
    void Render(
        const SceneState&    state,
        const DebugState&    debugState,
        const audio::ExperienceSignals& signals,
        const ExperiencePreset& preset,
        const visual::Theme& theme) override;
    [[nodiscard]] bool EndFrame() override;
    void Shutdown() noexcept override;

    [[nodiscard]] int ConsumeThemeRequest() noexcept override;
    [[nodiscard]] bool ConsumePlayPauseToggle() noexcept override;
    [[nodiscard]] bool ConsumeReloadRequest() noexcept override;
    [[nodiscard]] bool ConsumeAutoToggle() noexcept override;

private:
    class Implementation;

    std::unique_ptr<Implementation> implementation_;
    ShaderManager shaderManager_;
    unsigned int  fullscreenVAO_ = 0u;
    bool vsyncEnabled_ = true;
    bool initialized_  = false;
};

} // namespace papagedon
