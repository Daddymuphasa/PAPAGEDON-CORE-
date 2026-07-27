#include "ExperiencePreset.h"

#include <array>

namespace papagedon {

// ──────────────────────────────────────────────────────────────────────────────
// Built-in preset table
//
// The single source of truth for every preset.  Declared constexpr so it lives
// in read-only storage: no construction cost, no heap, and switching presets is
// just an index change (see PresetManager).  Each entry is a self-contained
// description of a visual style — colours plus response curves — with zero
// rendering logic.
// ──────────────────────────────────────────────────────────────────────────────
namespace {

constexpr std::array<ExperiencePreset, kPresetCount> kPresets = {{
    // ── Aurora ──────────────────────────────────────────────────────────────
    // Cool flowing greens and teals bleeding into violet.  Gentle, wide motion.
    ExperiencePreset{
        .id               = PresetId::Aurora,
        .name             = "Aurora",
        .colorLow         = {0.00F, 0.15F, 0.20F},
        .colorMid         = {0.00F, 0.80F, 0.55F},
        .colorHigh        = {0.55F, 0.30F, 0.85F},
        .background       = {0.02F, 0.04F, 0.08F},
        .patternMode      = PatternMode::AuroraCurtains,
        .warp             = 1.20F,
        .detail           = 1.00F,
        .beatResponse     = 0.70F,
        .energyMultiplier = 1.00F,
        .saturationBase   = 0.35F,
        .saturationScale  = 0.65F,
        .motionIntensity  = 0.70F,
        .transitionSpeed  = 3.5F,
    },
    // ── Nebula ──────────────────────────────────────────────────────────────
    // Deep cosmic indigo through magenta to a blue-white core.  Slow drift.
    ExperiencePreset{
        .id               = PresetId::Nebula,
        .name             = "Nebula",
        .colorLow         = {0.08F, 0.02F, 0.20F},
        .colorMid         = {0.55F, 0.10F, 0.65F},
        .colorHigh        = {0.30F, 0.55F, 0.95F},
        .background       = {0.04F, 0.01F, 0.07F},
        .patternMode      = PatternMode::NebulaClouds,
        .warp             = 1.60F,
        .detail           = 1.20F,
        .beatResponse     = 0.60F,
        .energyMultiplier = 0.90F,
        .saturationBase   = 0.40F,
        .saturationScale  = 0.60F,
        .motionIntensity  = 0.50F,
        .transitionSpeed  = 3.0F,
    },
    // ── Matrix ──────────────────────────────────────────────────────────────
    // High-contrast digital green on near-black.  Sharp beats, quick motion.
    ExperiencePreset{
        .id               = PresetId::Matrix,
        .name             = "Matrix",
        .colorLow         = {0.00F, 0.10F, 0.00F},
        .colorMid         = {0.00F, 0.70F, 0.15F},
        .colorHigh        = {0.60F, 1.00F, 0.60F},
        .background       = {0.00F, 0.02F, 0.00F},
        .patternMode      = PatternMode::MatrixRain,
        .warp             = 0.40F,
        .detail           = 0.80F,
        .beatResponse     = 1.20F,
        .energyMultiplier = 1.10F,
        .saturationBase   = 0.20F,
        .saturationScale  = 0.50F,
        .motionIntensity  = 1.30F,
        .transitionSpeed  = 6.0F,
    },
    // ── Liquid ──────────────────────────────────────────────────────────────
    // Smooth blues and cyans lifting to white.  Soft beats, languid motion.
    ExperiencePreset{
        .id               = PresetId::Liquid,
        .name             = "Liquid",
        .colorLow         = {0.00F, 0.10F, 0.25F},
        .colorMid         = {0.00F, 0.55F, 0.85F},
        .colorHigh        = {0.75F, 0.95F, 1.00F},
        .background       = {0.00F, 0.03F, 0.06F},
        .patternMode      = PatternMode::LiquidFlow,
        .warp             = 1.40F,
        .detail           = 0.90F,
        .beatResponse     = 0.40F,
        .energyMultiplier = 0.95F,
        .saturationBase   = 0.45F,
        .saturationScale  = 0.55F,
        .motionIntensity  = 0.50F,
        .transitionSpeed  = 2.5F,
    },
    // ── Tunnel ──────────────────────────────────────────────────────────────
    // Hot orange-to-yellow rush.  High energy, very fast forward motion.
    ExperiencePreset{
        .id               = PresetId::Tunnel,
        .name             = "Tunnel",
        .colorLow         = {0.15F, 0.02F, 0.00F},
        .colorMid         = {0.95F, 0.35F, 0.00F},
        .colorHigh        = {1.00F, 0.85F, 0.30F},
        .background       = {0.03F, 0.00F, 0.00F},
        .patternMode      = PatternMode::WarpTunnel,
        .warp             = 1.00F,
        .detail           = 1.00F,
        .beatResponse     = 1.00F,
        .energyMultiplier = 1.20F,
        .saturationBase   = 0.35F,
        .saturationScale  = 0.65F,
        .motionIntensity  = 1.80F,
        .transitionSpeed  = 7.0F,
    },
    // ── Pulse ───────────────────────────────────────────────────────────────
    // Punchy reds and pinks flaring to white.  The most beat-reactive preset.
    ExperiencePreset{
        .id               = PresetId::Pulse,
        .name             = "Pulse",
        .colorLow         = {0.20F, 0.00F, 0.05F},
        .colorMid         = {0.95F, 0.10F, 0.35F},
        .colorHigh        = {1.00F, 0.85F, 0.90F},
        .background       = {0.05F, 0.00F, 0.02F},
        .patternMode      = PatternMode::RadialPulse,
        .warp             = 0.80F,
        .detail           = 1.10F,
        .beatResponse     = 1.60F,
        .energyMultiplier = 1.30F,
        .saturationBase   = 0.30F,
        .saturationScale  = 0.70F,
        .motionIntensity  = 1.40F,
        .transitionSpeed  = 8.0F,
    },
}};

} // namespace

const ExperiencePreset& GetPreset(const PresetId id) noexcept {
    const auto index = static_cast<std::size_t>(id);
    return kPresets[index < kPresetCount ? index : 0u];
}

const ExperiencePreset& DefaultPreset() noexcept {
    return kPresets[static_cast<std::size_t>(PresetId::Aurora)];
}

} // namespace papagedon
