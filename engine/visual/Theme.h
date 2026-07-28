#pragma once

#include "ThemePalette.h"

#include <string>
#include <string_view>
#include <unordered_map>

namespace papagedon::visual {

// ──────────────────────────────────────────────────────────────────────────────
// Theme
//
// A Theme defines the *visual identity* of an experience — the look that makes a
// Cyberpunk night read differently from an Industrial warehouse — as pure,
// serializable data.  It contains no rendering code and no audio state.
//
// The same music, rendered under two different themes, produces two completely
// different visual identities: the Renderer combines the live ExperienceSignals
// with the Theme when it builds ShaderUniforms.  Themes are orthogonal to the
// ExperiencePreset "form" axis — a theme recolours and re-styles whatever form
// is currently playing.
// ──────────────────────────────────────────────────────────────────────────────

/// Ambient particle character of a theme.
enum class ParticleStyle { None, Sparks, Embers, Dust, Snow, Rain, Bokeh, Confetti };

/// How motion feels — pacing and attack of the animation.
enum class MotionStyle { Smooth, Flowing, Pulsing, Aggressive, Strobing, Hypnotic };

/// The dominant geometric character of the imagery.
enum class GeometryStyle { Organic, Grid, Radial, Fractal, Tunnel, Waves };

/// Grain / texture overlay style.
enum class NoiseStyle { None, Film, Digital, Turbulent, Scanline };

/// How the visuals change when switching to this theme.
enum class TransitionStyle { Cut, Fade, Dissolve, Wipe, Glitch };

struct Theme final {
    std::string id;    ///< Stable machine identifier, e.g. "cyberpunk".
    std::string name;  ///< Human-readable display name, e.g. "Cyberpunk".

    // ── Colour identity ────────────────────────────────────────────────────────
    ThemePalette palette;

    // ── Look ───────────────────────────────────────────────────────────────────
    /// Contrast of the final image (1 = neutral, >1 harder, <1 softer).
    float contrast = 1.0F;

    /// Overall additive glow lift (0 = off).
    float glowStrength = 0.0F;

    /// Highlight bloom strength (0 = off; 0.35 matches the engine default).
    float bloomStrength = 0.35F;

    // ── Style axes (data — consumed by the renderer / future subsystems) ────────
    ParticleStyle   particleStyle   = ParticleStyle::None;
    MotionStyle     motionStyle     = MotionStyle::Smooth;
    GeometryStyle   geometryStyle   = GeometryStyle::Organic;
    NoiseStyle      noiseStyle      = NoiseStyle::None;
    TransitionStyle transitionStyle = TransitionStyle::Fade;

    // ── Free-form tunables ──────────────────────────────────────────────────────
    /// Named scalar parameters a theme can carry for shaders/effects that opt in.
    /// Kept generic so new effects need no change to the Theme contract.
    std::unordered_map<std::string, float> shaderParameters;
};

// ──────────────────────────────────────────────────────────────────────────────
// Enum ⇄ string (used by JSON and tooling)
// ──────────────────────────────────────────────────────────────────────────────
[[nodiscard]] std::string_view ToString(ParticleStyle) noexcept;
[[nodiscard]] std::string_view ToString(MotionStyle) noexcept;
[[nodiscard]] std::string_view ToString(GeometryStyle) noexcept;
[[nodiscard]] std::string_view ToString(NoiseStyle) noexcept;
[[nodiscard]] std::string_view ToString(TransitionStyle) noexcept;

[[nodiscard]] bool FromString(std::string_view, ParticleStyle&) noexcept;
[[nodiscard]] bool FromString(std::string_view, MotionStyle&) noexcept;
[[nodiscard]] bool FromString(std::string_view, GeometryStyle&) noexcept;
[[nodiscard]] bool FromString(std::string_view, NoiseStyle&) noexcept;
[[nodiscard]] bool FromString(std::string_view, TransitionStyle&) noexcept;

// ──────────────────────────────────────────────────────────────────────────────
// JSON serialization
//
// Self-contained (no third-party dependency).  Colours are written as "#RRGGBB"
// hex strings for authoring friendliness; on read, a colour may be either a hex
// string or a [r, g, b] array of floats in [0, 1].
// ──────────────────────────────────────────────────────────────────────────────

/// Serializes a theme to pretty-printed JSON.
[[nodiscard]] std::string ToJson(const Theme& theme);

/// Parses a theme from a JSON document.  Returns false on malformed input, and
/// on failure writes a human-readable reason to `error` when it is non-null;
/// `out` is left unchanged in that case.
[[nodiscard]] bool FromJson(std::string_view json, Theme& out, std::string* error = nullptr);

} // namespace papagedon::visual
