#pragma once

namespace papagedon {

/// A single RGB colour with components normalised to [0, 1].
struct Color3 final {
    float r = 0.0F;
    float g = 0.0F;
    float b = 0.0F;
};

// ──────────────────────────────────────────────────────────────────────────────
// ShaderUniforms
//
// All per-frame data passed to the fullscreen shader.  Populated by the Runtime
// from ExperienceSignals + SceneState.  The shader layer itself knows nothing
// about audio or experience types — it only receives floats.
//
// Resolution and time are injected by the renderer, not the runtime.
// ──────────────────────────────────────────────────────────────────────────────
struct ShaderUniforms final {
    /// Normalised energy level [0, 1].  Maps to HSV value (brightness).
    float energy    = 0.0F;

    /// Normalised intensity [0, 1].  Maps to HSV saturation.
    float intensity = 0.0F;

    /// Low-frequency band proxy [0, 1].  Placeholder until band analysis lands.
    float bass      = 0.0F;

    /// Mid-frequency band proxy [0, 1].  Placeholder until band analysis lands.
    float mid       = 0.0F;

    /// High-frequency band proxy [0, 1].  Placeholder until band analysis lands.
    float treble    = 0.0F;

    /// 1.0 on a beat frame, 0.0 otherwise.
    float beat      = 0.0F;

    /// Tonal warmth [0 = cool, 1 = warm].  Forwarded from the active scene.
    float mood      = 0.0F;

    // ── Experience-preset palette ───────────────────────────────────────────────
    // The active preset's colour palette, eased toward on the CPU when the preset
    // changes (by ExperiencePreset::transitionSpeed) before it reaches the shader.
    // The fragment shader maps its pattern intensity across this three-stop ramp,
    // so every preset reads as a distinct colour world and switches cross-fade.
    Color3 colorLow;   ///< Ramp stop for the darkest pattern regions.
    Color3 colorMid;   ///< Ramp stop for mid-intensity regions.
    Color3 colorHigh;  ///< Ramp stop for the brightest regions.

    // ── Experience-preset behaviour ─────────────────────────────────────────────
    /// Ambient base colour that fills the darkest regions of the frame.
    Color3 background;

    /// Saturation curve constants: saturation = base + intensity * scale.
    float saturationBase  = 0.15F;
    float saturationScale = 0.85F;

    /// Animation-speed / pattern-motion multiplier (1 = nominal).
    float motion          = 1.0F;

    // ── Experience-preset form ──────────────────────────────────────────────────
    // Which signature pattern the shader draws.  During a preset switch the
    // Renderer cross-fades from previousPatternMode to patternMode as
    // patternBlend eases 0 → 1, so the visual *form* morphs instead of popping.
    int   patternMode         = 0;  ///< Incoming/active PatternMode as an int.
    int   previousPatternMode = 0;  ///< Outgoing PatternMode during a switch.
    float patternBlend        = 1.0F; ///< 0 = previous form, 1 = active form.

    /// Domain-warp strength — higher is more turbulent / distorted.
    float warp   = 1.0F;

    /// Fractal detail emphasis — higher retains more fine high-frequency structure.
    float detail = 1.0F;
};

} // namespace papagedon
