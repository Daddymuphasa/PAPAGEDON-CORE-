#pragma once

namespace papagedon::visual {

// ──────────────────────────────────────────────────────────────────────────────
// ThemeColor / ThemePalette
//
// The colour identity of a Theme, expressed as pure data.  Components are
// normalised to [0, 1] so the renderer can hand them straight to the shader
// without conversion.  This header is deliberately dependency-free — the visual
// identity layer never needs to know about the renderer's own colour types.
// ──────────────────────────────────────────────────────────────────────────────

/// A single RGB colour, components in [0, 1].
struct ThemeColor final {
    float r = 0.0F;
    float g = 0.0F;
    float b = 0.0F;
};

/// The four defining colours of a Theme.
///
///   primary     — the dominant hue that carries the identity.
///   secondary   — supporting hue filling the shadows / mid tones.
///   accent      — high-energy highlight colour (beats, bright structure).
///   background  — ambient tone that fills the darkest regions of the frame.
struct ThemePalette final {
    ThemeColor primary;
    ThemeColor secondary;
    ThemeColor accent;
    ThemeColor background;
};

} // namespace papagedon::visual
