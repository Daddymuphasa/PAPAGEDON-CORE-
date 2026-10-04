#pragma once

#include "ExperienceTypes.h"
#include "ShowMode.h"
#include <presets/ExperiencePreset.h>

#include <cstdint>

namespace papagedon {

// ──────────────────────────────────────────────────────────────────────────────
// AutoDirector — the heuristic "Auto-VJ".
//
// Given the live ExperienceGraph decision each frame, it chooses which preset
// should be on screen, the way a VJ runs a live DJ set: hold a look for a while,
// hard-cut on a drop, escalate the visual with the energy, and rotate for
// variety across a long set.
//
// Built for live performance:
//   - Source-agnostic. It consumes only the analysed experience, so it behaves
//     identically whether the audio is a decoded file or a live line-in from a
//     DJ mixer — the audio front-end can change without touching this class.
//   - No look-ahead and no per-track state. A live set has no known structure,
//     length or beat map, so every decision is made from the current signal and
//     a short rolling energy envelope only.
//   - Stable under a continuous stream: a minimum dwell time stops the visual
//     from flickering, while drops still cut instantly for impact.
//
// Real-time safe: only integer/float state, no heap allocations.
// ──────────────────────────────────────────────────────────────────────────────
class AutoDirector final {
public:
    AutoDirector() noexcept = default;

    /// Advances the director by one frame and returns the preset to display.
    [[nodiscard]] PresetId Update(const ExperienceGraphOutput& experience,
                                  float deltaSeconds) noexcept;

    /// The preset currently selected by the director.
    [[nodiscard]] PresetId Current() const noexcept { return current_; }

    /// Clears all state (e.g. when re-arming for a new set).
    void Reset() noexcept;
    void SetMode(ShowMode mode) noexcept;

private:
    [[nodiscard]] int      TierFor(float energyEnvelope, ExperienceState state) const noexcept;
    [[nodiscard]] PresetId PickFromTier(int tier, PresetId avoid) noexcept;
    [[nodiscard]] std::uint32_t NextRandom() noexcept;

    PresetId        current_     = PresetId::Aurora;
    int             currentTier_ = 0;
    float           energyEnv_   = 0.0F;
    float           dwell_       = 0.0F;
    ExperienceEvent lastEvent_   = ExperienceEvent::Silence;
    std::uint32_t   rng_         = 0x9E3779B9u;
    bool            initialized_ = false;
    ShowMode        mode_ = ShowMode::Open;

    // Smoothed frequency bands for stable matching decisions.
    float smoothBass_   = 0.0F;
    float smoothTreble_ = 0.0F;
    float smoothMood_   = 0.0F;
};

} // namespace papagedon
