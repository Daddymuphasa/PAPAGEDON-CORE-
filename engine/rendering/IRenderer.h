#pragma once

#include "../audio/ExperienceSignals.h"

#include <presets/ExperiencePreset.h>

namespace papagedon {

namespace visual { struct Theme; }

struct SceneState;
struct DebugState;

/// Interface implemented by each graphics backend.
class IRenderer {
public:
    virtual ~IRenderer() = default;

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

    /// True once per Space press — toggles audio play/pause.
    [[nodiscard]] virtual bool ConsumePlayPauseToggle() noexcept = 0;

    /// True once per R press — reload the current theme's JSON from disk.
    [[nodiscard]] virtual bool ConsumeReloadRequest() noexcept = 0;

    /// True once per A press — toggles Auto-VJ (automatic preset/form selection).
    [[nodiscard]] virtual bool ConsumeAutoToggle() noexcept = 0;
};

} // namespace papagedon
