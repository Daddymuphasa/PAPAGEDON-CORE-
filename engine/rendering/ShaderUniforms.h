#pragma once

namespace papagedon {

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
};

} // namespace papagedon
