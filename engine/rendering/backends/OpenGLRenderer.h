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

    void Configure(bool fullscreen, bool vsync) override;
    void SetMasterControls(float brightness, float glow, float exposure) override;
    void SetDemoMode(bool enabled) override;
    void SetDebugOverlay(bool visible) override;
    void PresentSplash(const std::string& status, float progress) override;
    [[nodiscard]] const char* BackendName() const noexcept override;

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
    [[nodiscard]] bool ConsumeTranceRequest() noexcept override;
    [[nodiscard]] bool ConsumeBadmanRequest() noexcept override;
    [[nodiscard]] bool ConsumeInputSwitchRequest() noexcept override;
    [[nodiscard]] bool ConsumePlayPauseToggle() noexcept override;
    [[nodiscard]] bool ConsumeReloadRequest() noexcept override;
    [[nodiscard]] bool ConsumeAutoToggle() noexcept override;
    [[nodiscard]] std::string ConsumeDroppedFile() noexcept override;

private:
    class Implementation;

    /// Switches between fullscreen and windowed, preserving windowed geometry.
    void SetFullscreen(bool enable);

    /// Begins a dramatic randomised crossfade transition to the given library shader index.
    void BeginShaderTransition(int target);

    std::unique_ptr<Implementation> implementation_;
    ShaderManager shaderManager_;
    unsigned int  fullscreenVAO_ = 0u;
    bool vsyncEnabled_        = true;
    bool fullscreenRequested_ = false;
    bool initialized_         = false;
};

} // namespace papagedon
