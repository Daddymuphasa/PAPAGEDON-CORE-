#include "AutoDirector.h"

#include <array>
#include <cmath>
#include <cstddef>

namespace papagedon {

namespace {

// ── Preset tiers ────────────────────────────────────────────────────────────
// A VJ escalates the visual with the music.  Presets are grouped by intensity:
//   0 = calm / breakdown / ambient
//   1 = groove / building
//   2 = peak / drop
constexpr std::array<PresetId, 3> kCalmTier{{
    PresetId::Aurora, PresetId::Nebula, PresetId::Liquid,
}};
constexpr std::array<PresetId, 5> kGrooveTier{{
    PresetId::Matrix, PresetId::Mandala, PresetId::Lattice,
    PresetId::Spectrum, PresetId::Vortex,
}};
constexpr std::array<PresetId, 4> kPeakTier{{
    PresetId::Shockwave, PresetId::Lasers, PresetId::Pulse, PresetId::Tunnel,
}};

// ── Timing (seconds) ────────────────────────────────────────────────────────
// Tuned for a live set: hold a look long enough to read, but keep it moving.
constexpr float kMinDwell   = 6.0F;   // min time before a section-change switch
constexpr float kMaxDwell   = 14.0F;  // force a variety switch by here
constexpr float kDropCutMin = 3.0F;   // min gap before a drop may re-cut

[[nodiscard]] std::size_t TierSize(const int tier) noexcept {
    if (tier == 0) return kCalmTier.size();
    if (tier == 1) return kGrooveTier.size();
    return kPeakTier.size();
}

[[nodiscard]] PresetId TierAt(const int tier, const std::size_t i) noexcept {
    if (tier == 0) return kCalmTier[i];
    if (tier == 1) return kGrooveTier[i];
    return kPeakTier[i];
}

} // namespace

std::uint32_t AutoDirector::NextRandom() noexcept {
    // xorshift32 — deterministic and allocation-free.
    rng_ ^= rng_ << 13;
    rng_ ^= rng_ >> 17;
    rng_ ^= rng_ << 5;
    return rng_;
}

int AutoDirector::TierFor(const float env, const ExperienceState state) const noexcept {
    if (state == ExperienceState::Silence || env < 0.12F) return 0;
    if (state == ExperienceState::Drop    || env >= 0.55F) return 2;
    return 1;
}

PresetId AutoDirector::PickFromTier(const int tier, const PresetId avoid) noexcept {
    const std::size_t n = TierSize(tier);
    if (n <= 1) return TierAt(tier, 0);

    // Pick a random slot, retrying a few times to avoid repeating the current
    // preset so a switch is always visible.
    PresetId choice = avoid;
    for (int attempt = 0; attempt < 8 && choice == avoid; ++attempt) {
        choice = TierAt(tier, NextRandom() % n);
    }
    return choice;
}

PresetId AutoDirector::Update(const ExperienceGraphOutput& e, const float dt) noexcept {
    if (!initialized_) {
        currentTier_ = TierFor(e.energy, e.state);
        current_     = PickFromTier(currentTier_, PresetId::Aurora);
        energyEnv_   = e.energy;
        initialized_ = true;
        return current_;
    }

    // A slow energy envelope (~0.7 s) keeps tier decisions from chattering on
    // momentary dips — important on a raw live signal.
    const float alpha = 1.0F - std::exp(-dt * 1.5F);
    energyEnv_ += (e.energy - energyEnv_) * alpha;
    dwell_     += dt;

    const bool dropEdge = (e.event == ExperienceEvent::Drop &&
                           lastEvent_ != ExperienceEvent::Drop);
    lastEvent_ = e.event;

    const int tier = TierFor(energyEnv_, e.state);

    bool switchNow = false;
    if (dropEdge && dwell_ >= kDropCutMin) {
        switchNow = true;                 // hit the drop
    } else if (dwell_ >= kMinDwell && tier != currentTier_) {
        switchNow = true;                 // the section changed
    } else if (dwell_ >= kMaxDwell) {
        switchNow = true;                 // keep it fresh over a long set
    }

    if (switchNow) {
        currentTier_ = tier;
        current_     = PickFromTier(tier, current_);
        dwell_       = 0.0F;
    }
    return current_;
}

void AutoDirector::Reset() noexcept {
    current_     = PresetId::Aurora;
    currentTier_ = 0;
    energyEnv_   = 0.0F;
    dwell_       = 0.0F;
    lastEvent_   = ExperienceEvent::Silence;
    initialized_ = false;
}

} // namespace papagedon
