#pragma once

#include "Theme.h"

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace papagedon::visual {

// ──────────────────────────────────────────────────────────────────────────────
// ThemeManager
//
// Owns the registry of available themes and the single active selection.  It
// starts pre-populated with the built-in themes (Cyberpunk, Neon Rave, Minimal,
// Industrial, Psychedelic, Dark Techno) so a valid theme is always current.
//
// Themes can also be loaded from JSON on disk (LoadTheme / ReloadTheme), which is
// how event-specific identities are authored without recompiling.  Switching the
// active theme is a trivial index change — no allocation — so the Runtime can
// hand CurrentTheme() to the Renderer every frame and swap identities instantly.
// ──────────────────────────────────────────────────────────────────────────────
class ThemeManager final {
public:
    /// Registers the built-in themes and selects the default (Cyberpunk).
    ThemeManager();

    /// Loads a theme from a JSON file.  On success the theme is added to the
    /// registry (replacing any existing theme with the same id), becomes the
    /// current selection, and its source path is remembered for ReloadTheme().
    /// On failure the registry and current selection are left unchanged; when
    /// `error` is non-null a human-readable reason is written to it.
    bool LoadTheme(const std::string& path, std::string* error = nullptr);

    /// Selects a registered theme by id.  Returns false (and keeps the current
    /// selection) if no theme with that id exists.
    bool SetTheme(std::string_view id) noexcept;

    /// The active theme.  Always valid.
    [[nodiscard]] const Theme& CurrentTheme() const noexcept;

    /// The id of the active theme.
    [[nodiscard]] std::string_view CurrentId() const noexcept;

    /// Re-reads the current theme from the file it was loaded from.  Returns
    /// false if the current theme is a built-in (no source file) or the reload
    /// fails; in that case the theme is left unchanged.
    bool ReloadTheme(std::string* error = nullptr);

    /// Cycles the active theme forward / backward through the registry.
    void NextTheme() noexcept;
    void PreviousTheme() noexcept;

    /// Read-only access to the registry.
    [[nodiscard]] const std::vector<Theme>& Themes() const noexcept { return themes_; }
    [[nodiscard]] std::size_t ThemeCount() const noexcept { return themes_.size(); }

    /// Writes a theme to a JSON file.  Returns false on I/O error.
    static bool SaveTheme(const Theme& theme, const std::string& path);

private:
    void RegisterBuiltins();

    /// Adds a theme, or replaces the existing one with the same id.  Returns the
    /// index of the stored theme.
    std::size_t Upsert(Theme theme);

    std::vector<Theme> themes_;
    std::size_t currentIndex_ = 0;

    /// Source file of the current theme, if it was loaded from disk (enables
    /// ReloadTheme).  Empty when the current theme is a built-in.
    std::string currentSourcePath_;
};

} // namespace papagedon::visual
