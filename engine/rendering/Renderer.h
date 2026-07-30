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

    /// Demo-mode configuration — see IRenderer.
    void Configure(bool fullscreen, bool vsync);
    void SetMasterControls(float brightness, float glow, float exposure);
    void SetDemoMode(bool enabled);
    void SetDebugOverlay(bool visible);
    void PresentSplash(const std::string& status, float progress);
    [[nodiscard]] const char* BackendName() const noexcept;

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

    /// Theme slot requested via F1..F7 (0..6), or -1. See IRenderer.
    [[nodiscard]] int ConsumeThemeRequest() noexcept;
    [[nodiscard]] bool ConsumeHomeThemeRequest() noexcept;

    /// True once per Space press (play/pause). See IRenderer.
    [[nodiscard]] bool ConsumePlayPauseToggle() noexcept;

    /// True once per R press (reload theme JSON). See IRenderer.
    [[nodiscard]] bool ConsumeReloadRequest() noexcept;

    /// True once per A press (Auto-VJ toggle). See IRenderer.
    [[nodiscard]] bool ConsumeAutoToggle() noexcept;

private:
    std::unique_ptr<IRenderer> backend_;
};

} // namespace papagedon
