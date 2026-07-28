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

    /// Returns the preset index (0..kPresetCount-1) requested via the temporary
    /// F1..F12 controls since the previous call, or -1 if none.  Consuming the
    /// request clears it.  This is a Stage 5.3 stop-gap until a dedicated input
    /// system exists; keeping it generic avoids coupling the backend to preset
    /// semantics beyond "function key N was pressed".
    [[nodiscard]] virtual int ConsumePresetRequest() noexcept = 0;

    /// Returns true once for each press of the Auto-VJ toggle key ('A') since the
    /// previous call.  Consuming the event clears it.
    [[nodiscard]] virtual bool ConsumeAutoToggle() noexcept = 0;

    /// Returns true once for each press of the theme-cycle key ('T') since the
    /// previous call.  Consuming the event clears it.
    [[nodiscard]] virtual bool ConsumeThemeToggle() noexcept = 0;
};

} // namespace papagedon
