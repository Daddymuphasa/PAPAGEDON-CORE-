#pragma once

#include <string>

namespace papagedon::runtime {

// ──────────────────────────────────────────────────────────────────────────────
// DemoConfig
//
// Plain-data configuration for the presentation-ready Demo Mode, loaded from
// config/demo.json.  Every field has a safe default, so a missing or partial
// file still yields a valid, runnable configuration.  The Runtime persists the
// last selected theme / audio file back to the same file on shutdown.
//
// The JSON is flat (a single object of scalar values) and parsed by a small,
// self-contained reader — no third-party dependency.
// ──────────────────────────────────────────────────────────────────────────────
struct DemoConfig final {
    /// Presentation mode: fullscreen, cursor auto-hide, overlay suppressed.
    bool demoMode = true;

    bool fullscreen = true;
    bool vsync      = true;
    int  targetFPS  = 120;

    std::string theme     = "cyberpunk";  ///< Theme id to select on launch.
    std::string audioFile;                ///< Audio clip to auto-load ("" = none).

    /// Audio source: "file" (play audioFile), "input" (live capture device), or
    /// "loopback" (capture system output). Live modes react to the DJ booth.
    std::string audioSource   = "file";
    int         captureDevice = -1;        ///< Capture/output device index (-1 = default).

    bool showDebugOverlay = false;

    // ── Master output trims (operator globals; 1.0 = neutral) ───────────────────
    float masterBrightness = 1.0F;  ///< Final linear gain.
    float masterGlow       = 1.0F;  ///< Scales the theme glow contribution.
    float masterExposure   = 1.0F;  ///< Pre-bloom scene gain.

    /// Loads values from a JSON file.  Missing keys keep their default; a missing
    /// or unreadable file leaves every field at its default.  Returns true if the
    /// file was found and parsed, false otherwise (defaults still valid).
    bool Load(const std::string& path);

    /// Writes the current values to a JSON file.  Returns false on I/O error.
    [[nodiscard]] bool Save(const std::string& path) const;
};

} // namespace papagedon::runtime
