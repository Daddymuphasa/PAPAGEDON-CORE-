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
    // ── Vortex ────────────────────────────────────────────────────────────────
    // Hypnotic neon spiral wormhole, magenta through cyan, rushing inward.
    ExperiencePreset{
        .id               = PresetId::Vortex,
        .name             = "Vortex",
        .colorLow         = {0.30F, 0.00F, 0.50F},
        .colorMid         = {1.00F, 0.00F, 0.80F},
        .colorHigh        = {0.00F, 0.90F, 1.00F},
        .background       = {0.02F, 0.00F, 0.06F},
        .patternMode      = PatternMode::VortexTunnel,
        .warp             = 0.60F,
        .detail           = 0.90F,
        .beatResponse     = 1.30F,
        .energyMultiplier = 1.30F,
        .saturationBase   = 0.50F,
        .saturationScale  = 0.50F,
        .motionIntensity  = 1.40F,
        .transitionSpeed  = 6.0F,
    },
    // ── Lasers ────────────────────────────────────────────────────────────────
    // Sweeping laser-show fan on black, strobing green to white on the beat.
    ExperiencePreset{
        .id               = PresetId::Lasers,
        .name             = "Lasers",
        .colorLow         = {0.00F, 0.15F, 0.00F},
        .colorMid         = {0.10F, 1.00F, 0.20F},
        .colorHigh        = {0.80F, 1.00F, 0.90F},
        .background       = {0.00F, 0.02F, 0.00F},
        .patternMode      = PatternMode::LaserFan,
        .warp             = 0.20F,
        .detail           = 0.70F,
        .beatResponse     = 1.60F,
        .energyMultiplier = 1.30F,
        .saturationBase   = 0.45F,
        .saturationScale  = 0.55F,
        .motionIntensity  = 1.60F,
        .transitionSpeed  = 7.0F,
    },
    // ── Mandala ───────────────────────────────────────────────────────────────
    // Psychedelic mirrored kaleidoscope, jewel tones folding and rotating.
    ExperiencePreset{
        .id               = PresetId::Mandala,
        .name             = "Mandala",
        .colorLow         = {0.50F, 0.00F, 0.30F},
        .colorMid         = {1.00F, 0.60F, 0.00F},
        .colorHigh        = {0.00F, 0.80F, 1.00F},
        .background       = {0.03F, 0.00F, 0.03F},
        .patternMode      = PatternMode::Mandala,
        .warp             = 1.20F,
        .detail           = 1.10F,
        .beatResponse     = 1.20F,
        .energyMultiplier = 1.20F,
        .saturationBase   = 0.55F,
        .saturationScale  = 0.45F,
        .motionIntensity  = 1.00F,
        .transitionSpeed  = 5.0F,
    },
    // ── Lattice ───────────────────────────────────────────────────────────────
    // Pulsing neon lattice grid, cyan and magenta, scaling with the bass.
    ExperiencePreset{
        .id               = PresetId::Lattice,
        .name             = "Lattice",
        .colorLow         = {0.00F, 0.20F, 0.30F},
        .colorMid         = {0.00F, 0.90F, 1.00F},
        .colorHigh        = {1.00F, 0.20F, 0.90F},
        .background       = {0.00F, 0.02F, 0.04F},
        .patternMode      = PatternMode::NeonLattice,
        .warp             = 0.30F,
        .detail           = 0.80F,
        .beatResponse     = 1.50F,
        .energyMultiplier = 1.20F,
        .saturationBase   = 0.50F,
        .saturationScale  = 0.50F,
        .motionIntensity  = 1.20F,
        .transitionSpeed  = 6.5F,
    },
    // ── Shockwave ─────────────────────────────────────────────────────────────
    // Concentric bass shockwaves and radial rays exploding on the kick.
    ExperiencePreset{
        .id               = PresetId::Shockwave,
        .name             = "Shockwave",
        .colorLow         = {0.30F, 0.00F, 0.00F},
        .colorMid         = {1.00F, 0.30F, 0.00F},
        .colorHigh        = {1.00F, 0.90F, 0.40F},
        .background       = {0.03F, 0.00F, 0.00F},
        .patternMode      = PatternMode::BassShockwave,
        .warp             = 0.40F,
        .detail           = 0.90F,
        .beatResponse     = 1.70F,
        .energyMultiplier = 1.40F,
        .saturationBase   = 0.45F,
        .saturationScale  = 0.55F,
        .motionIntensity  = 1.30F,
        .transitionSpeed  = 7.5F,
    },
    // ── Spectrum ──────────────────────────────────────────────────────────────
    // Circular audio-reactive bars: bass, mid and treble arcs radiating outward.
    ExperiencePreset{
        .id               = PresetId::Spectrum,
        .name             = "Spectrum",
        .colorLow         = {0.10F, 0.00F, 0.40F},
        .colorMid         = {0.00F, 1.00F, 0.60F},
        .colorHigh        = {1.00F, 0.90F, 0.00F},
        .background       = {0.01F, 0.00F, 0.03F},
        .patternMode      = PatternMode::SpectrumRing,
        .warp             = 0.20F,
        .detail           = 0.80F,
        .beatResponse     = 1.40F,
        .energyMultiplier = 1.30F,
        .saturationBase   = 0.50F,
        .saturationScale  = 0.50F,
        .motionIntensity  = 1.00F,
        .transitionSpeed  = 6.0F,
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
