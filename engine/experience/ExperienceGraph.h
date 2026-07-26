#pragma once

#include "ExperienceTypes.h"
#include <ExperienceSignals.h>

namespace papagedon {

// ──────────────────────────────────────────────────────────────────────────────
// ExperienceGraph
//
// Consumes ExperienceSignals every frame and resolves high-level experience
// decisions.  It does not know about rendering or scene data.
// ──────────────────────────────────────────────────────────────────────────────
class ExperienceGraph final {
public:
    ExperienceGraph() = default;

    ExperienceGraph(const ExperienceGraph&) = delete;
    ExperienceGraph& operator=(const ExperienceGraph&) = delete;

    /// Processes one frame of signals and returns the resolved output.
    [[nodiscard]] ExperienceGraphOutput Update(
        const audio::ExperienceSignals& signals) noexcept;

    [[nodiscard]] const ExperienceGraphOutput& Latest() const noexcept;

private:
    [[nodiscard]] static ExperienceEvent ResolveEvent(
        const audio::ExperienceSignals& signals,
        float previousEnergy) noexcept;

    [[nodiscard]] static ExperienceState ResolveState(
        ExperienceEvent event) noexcept;

    [[nodiscard]] static float ResolveMood(
        ExperienceState state,
        float tension) noexcept;

    ExperienceGraphOutput latest_{};
    float previousEnergy_ = 0.0F;
};

} // namespace papagedon
