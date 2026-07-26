#pragma once

#include "SceneProfile.h"

#include <chrono>

namespace papagedon {

/// The current scene selection, its transition progress, and the live visual
/// parameters forwarded from the ExperienceGraph.  The Renderer consumes this
/// struct exclusively — it does not need to know about audio or experience types.
struct SceneState final {
    const SceneProfile* activeProfile  = nullptr;
    const SceneProfile* previousProfile = nullptr;
    float transitionProgress = 1.0F;
    std::chrono::steady_clock::time_point timestamp{};

    // ── Live visual parameters ────────────────────────────────────────────────
    /// Normalised energy level [0, 1].  Controls brightness (HSV value).
    float energy    = 0.0F;

    /// Normalised intensity [0, 1].  Controls colour saturation.
    float intensity = 0.0F;

    /// Tonal warmth [0 = cool, 1 = warm].  Shifts the hue selection.
    float mood      = 0.0F;
};

} // namespace papagedon
