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
// starts pre-populated with the seven built-in themes (Cyberpunk, Dark Techno,
// Industrial, Neon Rave, Aurora, Nebula, Matrix) in a fixed order, so F1..F7 map
// to stable slots and a valid theme is always current.
//
// Themes can be overridden / hot-reloaded from JSON on disk (LoadTheme,
// LoadThemesFromDirectory, ReloadTheme): loading a JSON theme replaces the
// built-in that shares its id in place, keeping the slot order intact and
// remembering the file so ReloadTheme() can re-read it live.  Switching the
// active theme is a trivial index change — no allocation — so the Runtime can
// hand CurrentTheme() to the Renderer every frame and swap identities instantly.
// ──────────────────────────────────────────────────────────────────────────────
class ThemeManager final {
public:
    /// Registers the built-in themes and selects the default (Cyberpunk).
    ThemeManager();

    /// Loads a theme from a JSON file.  On success the theme is added to the
    /// registry (replacing any existing theme with the same id, in place),
    /// becomes the current selection, and its source path is remembered so
    /// ReloadTheme() can re-read it.  On failure nothing changes; when `error`
    /// is non-null a human-readable reason is written to it.
    bool LoadTheme(const std::string& path, std::string* error = nullptr);

    /// Loads every "*.json" file in `dir`, upserting each by id (a built-in is
    /// replaced in place, preserving its slot; new ids are appended).  Returns
    /// the number of themes successfully loaded.  The current selection is not
    /// changed.  A missing directory is not an error — it returns 0.
    std::size_t LoadThemesFromDirectory(const std::string& dir);

    /// Selects a registered theme by id.  Returns false (current kept) if unknown.
    bool SetTheme(std::string_view id) noexcept;

    /// Selects a registered theme by slot index (F1..F7 → 0..6).  Returns false
    /// (current kept) if the index is out of range.
    bool SetThemeByIndex(std::size_t index) noexcept;

    /// The active theme.  Always valid.
    [[nodiscard]] const Theme& CurrentTheme() const noexcept;
    [[nodiscard]] std::string_view CurrentId() const noexcept;
    [[nodiscard]] std::size_t CurrentIndex() const noexcept { return currentIndex_; }

    /// Re-reads the current theme from the file it was loaded from.  Returns
    /// false if the current theme has no source file (a pure built-in) or the
    /// reload fails; the theme is left unchanged in that case.
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

    /// Adds a theme, or replaces the existing one with the same id in place.
    /// Returns the index of the stored theme.
    std::size_t Upsert(Theme theme, std::string sourcePath);

    std::vector<Theme>       themes_;       ///< Registry, F1..F7 order.
    std::vector<std::string> sourcePaths_;  ///< Parallel to themes_; "" = built-in.
    std::size_t              currentIndex_ = 0;
};

} // namespace papagedon::visual
