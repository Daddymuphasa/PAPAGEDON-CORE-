#pragma once

namespace papagedon {

// ──────────────────────────────────────────────────────────────────────────────
// ExperienceState
//
// The sustained scene state resolved from the current experience event.
// Defined in the scene layer because SceneDNA maps it directly to SceneProfiles.
// ──────────────────────────────────────────────────────────────────────────────
enum class ExperienceState {
    Calm,
    BuildUp,
    Drop,
    Silence,
    Ambient,
};

[[nodiscard]] const char* ToString(ExperienceState state) noexcept;

} // namespace papagedon
