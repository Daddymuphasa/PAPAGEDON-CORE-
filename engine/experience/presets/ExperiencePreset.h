#pragma once

#include <cstddef>

namespace papagedon {

// ──────────────────────────────────────────────────────────────────────────────
// ExperiencePreset
//
// A preset describes an immersive *visual style* — Aurora, Nebula, Matrix… — as
// pure, immutable data.  It contains NO rendering code: it only tells the
// Renderer how the shared ExperienceSignals pipeline should look and feel.
//
// The same song, played under two different presets, produces two noticeably
// different experiences because the Renderer combines the live audio signals
// with these parameters when it builds ShaderUniforms.
//
// Presets are literal types stored in a static, immutable table (see
// GetPreset()).  Switching presets only changes an index — it never allocates.
// ──────────────────────────────────────────────────────────────────────────────

/// A single RGB colour with components in [0, 1].
///
/// Deliberately independent of the renderer's own colour type so the experience
/// layer never has to depend on the rendering module.
struct PresetColor final {
    float r = 0.0F;
    float g = 0.0F;
    float b = 0.0F;
};

/// Stable identifier for each built-in preset.  The underlying values are
/// contiguous starting at zero so they double as indices into the preset table.
enum class PresetId : std::size_t {
    Aurora = 0,
    Nebula,
    Matrix,
    Liquid,
    Tunnel,
    Pulse,
    Vortex,
    Lasers,
    Mandala,
    Lattice,
    Shockwave,
    Spectrum,
};

/// Number of built-in presets.  Kept in sync with PresetId by GetPreset().
inline constexpr std::size_t kPresetCount = 12;

/// Signature visual form a preset renders.  This selects which pattern the
/// shader generates for the preset; the preset supplies only the choice (data),
/// never any rendering code.
enum class PatternMode : int {
    AuroraCurtains = 0,  ///< Vertical flowing curtains (northern-lights).
    NebulaClouds,        ///< Drifting volumetric clouds with sparkle.
    MatrixRain,          ///< Falling digital-rain columns.
    LiquidFlow,          ///< Smooth caustic / fluid ripples.
    WarpTunnel,          ///< Perspective tunnel rushing inward.
    RadialPulse,         ///< Kaleidoscopic radial shockwaves.
    VortexTunnel,        ///< Hypnotic neon spiral vortex.
    LaserFan,            ///< Sweeping strobing laser fan.
    Mandala,             ///< Psychedelic mirrored kaleidoscope.
    NeonLattice,         ///< Pulsing neon lattice grid.
    BassShockwave,       ///< Concentric bass shockwaves + rays.
    SpectrumRing,        ///< Circular audio-reactive spectrum bars.
};

struct ExperiencePreset final {
    PresetId    id   = PresetId::Aurora;
    const char* name = "";

    // ── Colour palette ─────────────────────────────────────────────────────────
    // Three ramp stops the Renderer maps across pattern intensity (dark → bright).
    PresetColor colorLow{};
    PresetColor colorMid{};
    PresetColor colorHigh{};

    /// Ambient background colour that fills the darkest regions of the frame.
    PresetColor background{};

    // ── Form ─────────────────────────────────────────────────────────────────
    /// Signature visual pattern this preset renders.
    PatternMode patternMode = PatternMode::AuroraCurtains;

    /// Domain-warp strength — higher is more turbulent / distorted.
    float warp = 1.0F;

    /// Fractal detail emphasis — higher retains more fine high-frequency structure.
    float detail = 1.0F;

    // ── Behaviour ────────────────────────────────────────────────────────────
    /// How strongly beats pulse the visuals (0 = beats ignored, 1 = nominal).
    float beatResponse = 1.0F;

    /// Scales the audio energy that drives overall brightness.
    float energyMultiplier = 1.0F;

    /// Saturation curve, evaluated as base + intensity * scale (clamped to [0, 1]).
    float saturationBase  = 0.15F;
    float saturationScale = 0.85F;

    /// Scales animation speed / pattern motion (1 = nominal).
    float motionIntensity = 1.0F;

    /// Rate at which the Renderer eases toward this preset when switching.
    /// Higher is snappier.  Used only for CPU-side smoothing — no allocation.
    float transitionSpeed = 6.0F;
};

/// Returns the immutable preset for the given id.  Out-of-range ids clamp to the
/// default preset, so the return value is always valid.
[[nodiscard]] const ExperiencePreset& GetPreset(PresetId id) noexcept;

/// Returns the default preset (Aurora).
[[nodiscard]] const ExperiencePreset& DefaultPreset() noexcept;

} // namespace papagedon
