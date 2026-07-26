#pragma once

#include <ExperienceState.h>

namespace papagedon {

// ──────────────────────────────────────────────────────────────────────────────
// ExperienceEvent
//
// A one-frame semantic label for what is happening in the music right now.
// Beat is a transient pulse; the others map to sustained ExperienceStates.
// ──────────────────────────────────────────────────────────────────────────────
enum class ExperienceEvent {
    Beat,
    BuildUp,
    Drop,
    Calm,
    Silence,
};

[[nodiscard]] const char* ToString(ExperienceEvent event) noexcept;

// ──────────────────────────────────────────────────────────────────────────────
// ExperienceGraphOutput
//
// Everything the Experience Graph resolves from one frame of ExperienceSignals.
// Consumed by SceneDNA — contains no audio or rendering types.
// ──────────────────────────────────────────────────────────────────────────────
struct ExperienceGraphOutput final {
    ExperienceEvent event   = ExperienceEvent::Silence;
    ExperienceState state   = ExperienceState::Ambient;

    /// Normalised amplitude of the current experience [0, 1].
    float intensity = 0.0F;

    /// Tonal warmth [0 = cool, 1 = warm].
    float mood      = 0.0F;

    /// Normalised energy level [0, 1].
    float energy    = 0.0F;
};

} // namespace papagedon
