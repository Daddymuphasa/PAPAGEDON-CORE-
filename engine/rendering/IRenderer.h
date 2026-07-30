#pragma once

#include "../audio/ExperienceSignals.h"

#include <presets/ExperiencePreset.h>

#include <string>

namespace papagedon {

namespace visual { struct Theme; }

struct SceneState;
struct DebugState;

/// Interface implemented by each graphics backend.
class IRenderer {
public:
    virtual ~IRenderer() = default;

    // ── Demo-mode configuration (applied before / around Initialize) ────────────
    /// Requests the initial window mode and vsync.  Must be called before
    /// Initialize().  A fullscreen request that cannot be honoured falls back to
    /// a window rather than failing.
    virtual void Configure(bool fullscreen, bool vsync) = 0;

    /// Sets the master output trims (operator globals; 1.0 = neutral).
    virtual void SetMasterControls(float brightness, float glow, float exposure) = 0;

    /// Enables/disables demo presentation mode (fullscreen + cursor auto-hide +
    /// overlay suppressed).  Safe to call before or after Initialize().
    virtual void SetDemoMode(bool enabled) = 0;

    /// Sets whether the debug overlay is visible.
    virtual void SetDebugOverlay(bool visible) = 0;

    /// Draws one splash frame (logo + version + status + progress bar) and
    /// presents it.  Requires the window/context to already exist (post
    /// Initialize()).  `progress` is in [0, 1].
    virtual void PresentSplash(const std::string& status, float progress) = 0;

    /// Human-readable renderer backend name, e.g. "OpenGL 4.6 Core".
    [[nodiscard]] virtual const char* BackendName() const noexcept = 0;

    virtual bool Initialize() = 0;
    virtual void BeginFrame() = 0;
    virtual void Render(
        const SceneState&    state,
        const DebugState&    debugState,
        const audio::ExperienceSignals& signals,
        const ExperiencePreset& preset,
        const visual::Theme& theme) = 0;
    /// Returns false once the backend has received a request to close.
    virtual bool EndFrame() = 0;
    virtual void Shutdown() noexcept = 0;

    // ── Live controls (temporary; drained by the Runtime each frame) ────────────
    // Rising edges are latched in EndFrame (where events are polled) and cleared
    // when consumed.  ESC (exit) and F12 (debug overlay) are handled inside the
    // backend and surface via EndFrame()/the overlay rather than these consumers.

    /// Theme slot requested via F1..F7 since the previous call (0..6), or -1.
    [[nodiscard]] virtual int ConsumeThemeRequest() noexcept = 0;

    /// True once per B press — jump back to the home theme (Badman red).
    [[nodiscard]] virtual bool ConsumeHomeThemeRequest() noexcept = 0;

    /// True once per Space press — toggles audio play/pause.
    [[nodiscard]] virtual bool ConsumePlayPauseToggle() noexcept = 0;

    /// True once per R press — reload the current theme's JSON from disk.
    [[nodiscard]] virtual bool ConsumeReloadRequest() noexcept = 0;

    /// True once per A press — toggles Auto-VJ (automatic preset/form selection).
    [[nodiscard]] virtual bool ConsumeAutoToggle() noexcept = 0;
};

} // namespace papagedon
