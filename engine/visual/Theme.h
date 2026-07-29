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
// with the Theme when it builds ShaderUniforms.  A theme recolours and re-styles
// whatever pattern form is currently playing.
//
// Every non-colour parameter is a plain float multiplier so a theme can be
// authored and hot-reloaded from JSON with no engine changes.
// ──────────────────────────────────────────────────────────────────────────────
struct Theme final {
    std::string id;    ///< Stable machine identifier, e.g. "cyberpunk".
    std::string name;  ///< Human-readable display name, e.g. "Cyberpunk".

    // ── Colour identity ────────────────────────────────────────────────────────
    ThemePalette palette;  ///< primary / secondary / accent / background.

    // ── Look multipliers ────────────────────────────────────────────────────────
    /// Additive glow intensity (0 = off).
    float glow = 0.0F;

    /// Highlight bloom strength (0 = off; 0.35 matches the engine default).
    float bloom = 0.35F;

    /// Animation-speed multiplier (1 = nominal).
    float motion = 1.0F;

    /// Film/digital grain amount (0 = clean).
    float noise = 0.0F;

    /// Domain-warp / distortion strength (1 = nominal).
    float distortion = 1.0F;

    /// How fast the image eases into this theme on a switch (higher = snappier).
    float transitionSpeed = 6.0F;

    // ── Free-form tunables ──────────────────────────────────────────────────────
    /// Named scalar parameters a theme can carry for effects that opt in.
    std::unordered_map<std::string, float> shaderParameters;
};

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
/// `out` is left unchanged in that case.  Fields absent from the document keep
/// their default value, so partial theme files are valid.
[[nodiscard]] bool FromJson(std::string_view json, Theme& out, std::string* error = nullptr);

} // namespace papagedon::visual
