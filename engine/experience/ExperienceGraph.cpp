#include "ExperienceGraph.h"
#include "ExperienceTypes.h"

#include <algorithm>
#include <cmath>

namespace papagedon {

// ──────────────────────────────────────────────────────────────────────────────
// ToString implementations
// ──────────────────────────────────────────────────────────────────────────────
const char* ToString(const ExperienceEvent event) noexcept {
    switch (event) {
    case ExperienceEvent::Beat:    return "Beat";
    case ExperienceEvent::BuildUp: return "BuildUp";
    case ExperienceEvent::Drop:    return "Drop";
    case ExperienceEvent::Calm:    return "Calm";
    case ExperienceEvent::Silence: return "Silence";
    }
    return "Silence";
}

// ──────────────────────────────────────────────────────────────────────────────
// ExperienceGraph::Update
// ──────────────────────────────────────────────────────────────────────────────
ExperienceGraphOutput ExperienceGraph::Update(
    const audio::ExperienceSignals& signals) noexcept {

    const ExperienceEvent event = ResolveEvent(signals, previousEnergy_);
    const ExperienceState state = ResolveState(event);
    const float mood            = ResolveMood(state, signals.tension);

    latest_ = ExperienceGraphOutput{
        .event     = event,
        .state     = state,
        .intensity = std::clamp(signals.intensity, 0.0F, 1.0F),
        .mood      = mood,
        .energy    = std::clamp(signals.energy, 0.0F, 1.0F),
    };

    previousEnergy_ = signals.energy;
    return latest_;
}

const ExperienceGraphOutput& ExperienceGraph::Latest() const noexcept {
    return latest_;
}

// ──────────────────────────────────────────────────────────────────────────────
// ResolveEvent
//
// Maps raw signals to a one-frame semantic event label.
//
// Priority order (highest first):
//   1. Silence  — near-zero energy
//   2. Drop     — very high energy
//   3. Beat     — on-beat transient (overrides BuildUp/Calm)
//   4. BuildUp  — moderate energy and rising
//   5. Calm     — fallback
// ──────────────────────────────────────────────────────────────────────────────
ExperienceEvent ExperienceGraph::ResolveEvent(
    const audio::ExperienceSignals& signals,
    const float previousEnergy) noexcept {

    constexpr float kSilenceThreshold = 0.05F;
    constexpr float kDropThreshold    = 0.70F;
    constexpr float kBuildUpThreshold = 0.40F;
    constexpr float kRisingDelta      = 0.02F;

    if (signals.energy < kSilenceThreshold) {
        return ExperienceEvent::Silence;
    }

    if (signals.energy >= kDropThreshold) {
        return ExperienceEvent::Drop;
    }

    if (signals.beat) {
        return ExperienceEvent::Beat;
    }

    const float delta = signals.energy - previousEnergy;
    if (signals.energy >= kBuildUpThreshold && delta >= kRisingDelta) {
        return ExperienceEvent::BuildUp;
    }

    return ExperienceEvent::Calm;
}

// ──────────────────────────────────────────────────────────────────────────────
// ResolveState
//
// Maps an ExperienceEvent to the sustained ExperienceState.
// Beat does not change the sustained state; it briefly pulses.
// ──────────────────────────────────────────────────────────────────────────────
ExperienceState ExperienceGraph::ResolveState(
    const ExperienceEvent event) noexcept {
    switch (event) {
    case ExperienceEvent::Drop:    return ExperienceState::Drop;
    case ExperienceEvent::BuildUp: return ExperienceState::BuildUp;
    case ExperienceEvent::Calm:    return ExperienceState::Calm;
    case ExperienceEvent::Silence: return ExperienceState::Silence;
    case ExperienceEvent::Beat:    return ExperienceState::Calm;
    }
    return ExperienceState::Ambient;
}

// ──────────────────────────────────────────────────────────────────────────────
// ResolveMood
//
// Returns a normalised warmth value [0 = cool/dark, 1 = warm/bright].
// Blended from a per-state base and the live tension signal.
// ──────────────────────────────────────────────────────────────────────────────
float ExperienceGraph::ResolveMood(
    const ExperienceState state,
    const float tension) noexcept {

    // Per-state base warmth values
    float baseMood = 0.0F;
    switch (state) {
    case ExperienceState::Drop:    baseMood = 0.90F; break;
    case ExperienceState::BuildUp: baseMood = 0.70F; break;
    case ExperienceState::Calm:    baseMood = 0.35F; break;
    case ExperienceState::Silence: baseMood = 0.10F; break;
    case ExperienceState::Ambient: baseMood = 0.40F; break;
    }

    // Tension nudges mood towards warmth
    constexpr float kTensionWeight = 0.25F;
    return std::clamp(baseMood + tension * kTensionWeight, 0.0F, 1.0F);
}

} // namespace papagedon
