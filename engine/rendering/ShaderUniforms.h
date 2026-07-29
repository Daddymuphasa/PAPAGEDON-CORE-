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
// All per-frame data passed to the fullscreen shader.  Populated by the Renderer
// from the live ExperienceSignals (audio), the active visual::Theme (colour
// identity + look), and the active ExperiencePreset (which signature form to
// draw).  The shader layer itself knows nothing about audio, themes, or presets —
// it only receives floats and colours.
//
// The Renderer holds one persistent instance and eases toward new targets every
// frame (frame-rate independent), so switches cross-fade and no buffer is
// allocated in the render loop.  Resolution and time are injected by the backend.
// ──────────────────────────────────────────────────────────────────────────────
struct ShaderUniforms final {
    // ── Audio (ExperienceSignals) ───────────────────────────────────────────────
    float energy    = 0.0F;  ///< Overall brightness driver [0, 1].
    float intensity = 0.0F;  ///< Saturation driver [0, 1].
    float bass      = 0.0F;  ///< Low band [0, 1].
    float mid       = 0.0F;  ///< Mid band [0, 1].
    float treble    = 0.0F;  ///< High band [0, 1].
    float beat      = 0.0F;  ///< Beat pulse (peaks on the beat frame, decays).
    float mood      = 0.0F;  ///< Warm/cool tint [0 = cool, 1 = warm].

    // ── Theme colour identity ───────────────────────────────────────────────────
    // The palette maps across the pattern's dark → bright ramp as
    // secondary → primary → accent, with `background` filling the darkest regions.
    Color3 primaryColor;
    Color3 secondaryColor;
    Color3 accentColor;
    Color3 background;

    // ── Theme look multipliers ──────────────────────────────────────────────────
    float glow       = 0.0F;   ///< Additive glow lift (0 = off).
    float bloom      = 0.35F;  ///< Highlight bloom strength.
    float motion     = 1.0F;   ///< Animation-speed multiplier.
    float noise      = 0.0F;   ///< Grain amount (0 = clean).
    float distortion = 1.0F;   ///< Domain-warp strength.

    // ── Preset form & behaviour ─────────────────────────────────────────────────
    float saturationBase  = 0.15F;
    float saturationScale = 0.85F;
    int   patternMode         = 0;    ///< Active PatternMode as an int.
    int   previousPatternMode = 0;    ///< Outgoing PatternMode during a switch.
    float patternBlend        = 1.0F; ///< 0 = previous form, 1 = active form.
    float detail              = 1.0F; ///< Fractal detail emphasis.

    // ── Master output trims (Demo Mode operator globals; 1.0 = neutral) ─────────
    float masterBrightness = 1.0F;  ///< Final linear gain.
    float masterGlow       = 1.0F;  ///< Scales the glow contribution.
    float masterExposure   = 1.0F;  ///< Pre-bloom scene gain.
};

} // namespace papagedon
