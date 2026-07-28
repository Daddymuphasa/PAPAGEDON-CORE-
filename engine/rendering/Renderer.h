#pragma once

#include "IRenderer.h"
#include "../audio/ExperienceSignals.h"

#include <memory>

namespace papagedon {

namespace visual { struct Theme; }

struct SceneState;
struct DebugState;

/// Backend-neutral renderer facade.
class Renderer final {
public:
    Renderer();
    ~Renderer();

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    bool Initialize();
    void BeginFrame();
    void Render(
        const SceneState&    state,
        const DebugState&    debugState,
        const audio::ExperienceSignals& signals,
        const ExperiencePreset& preset,
        const visual::Theme& theme);
    /// Returns false once the active backend requests application shutdown.
    [[nodiscard]] bool EndFrame();
    void Shutdown() noexcept;

    /// Preset index requested via the temporary F1..F12 controls, or -1 if none.
    /// See IRenderer::ConsumePresetRequest.
    [[nodiscard]] int ConsumePresetRequest() noexcept;

    /// True once per press of the Auto-VJ toggle key. See IRenderer::ConsumeAutoToggle.
    [[nodiscard]] bool ConsumeAutoToggle() noexcept;

    /// True once per press of the theme-cycle key. See IRenderer::ConsumeThemeToggle.
    [[nodiscard]] bool ConsumeThemeToggle() noexcept;

private:
    std::unique_ptr<IRenderer> backend_;
};

} // namespace papagedon
