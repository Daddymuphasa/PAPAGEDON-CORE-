#include "ThemeManager.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <system_error>
#include <utility>

namespace papagedon::visual {
namespace {

/// Builds a normalised ThemeColor from a packed 0xRRGGBB literal.
constexpr ThemeColor Rgb(const unsigned int hex) noexcept {
    return ThemeColor{
        static_cast<float>((hex >> 16) & 0xFFu) / 255.0F,
        static_cast<float>((hex >> 8) & 0xFFu) / 255.0F,
        static_cast<float>(hex & 0xFFu) / 255.0F,
    };
}

bool ReadFile(const std::string& path, std::string& out) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return false;
    }
    std::ostringstream contents;
    contents << file.rdbuf();
    out = contents.str();
    return true;
}

} // namespace

ThemeManager::ThemeManager() {
    RegisterBuiltins();
}

// ──────────────────────────────────────────────────────────────────────────────
// Built-in themes — F1..F7 order
//
// Seven distinct visual identities.  Each maps its palette across the shader's
// dark → bright ramp (secondary → primary → accent) with `background` filling the
// darkest regions, and tunes glow / bloom / motion / noise / distortion for its
// mood.  These double as the fallback when no JSON theme files are present.
// ──────────────────────────────────────────────────────────────────────────────
void ThemeManager::RegisterBuiltins() {
    themes_.clear();
    sourcePaths_.clear();

    const auto add = [&](Theme t) {
        themes_.push_back(std::move(t));
        sourcePaths_.emplace_back();
    };

    {   // F1 — Cyberpunk: electric cyan and magenta over deep indigo.
        Theme t;
        t.id = "cyberpunk"; t.name = "Cyberpunk";
        t.palette.primary = Rgb(0x00E5FF); t.palette.secondary = Rgb(0x7B2CBF);
        t.palette.accent = Rgb(0xFF2A6D);  t.palette.background = Rgb(0x05010F);
        t.glow = 0.50F; t.bloom = 0.55F; t.motion = 1.10F; t.noise = 0.12F;
        t.distortion = 1.20F; t.transitionSpeed = 8.0F;
        add(std::move(t));
    }
    {   // F2 — Dark Techno: cold blues and a blood-red accent, moody, low-key.
        Theme t;
        t.id = "dark-techno"; t.name = "Dark Techno";
        t.palette.primary = Rgb(0xAEB6C2); t.palette.secondary = Rgb(0x1B2A4A);
        t.palette.accent = Rgb(0xE23B3B);  t.palette.background = Rgb(0x040406);
        t.glow = 0.22F; t.bloom = 0.22F; t.motion = 0.90F; t.noise = 0.15F;
        t.distortion = 1.00F; t.transitionSpeed = 5.0F;
        add(std::move(t));
    }
    {   // F3 — Industrial: steel and amber, hard and grainy.
        Theme t;
        t.id = "industrial"; t.name = "Industrial";
        t.palette.primary = Rgb(0xFFB000); t.palette.secondary = Rgb(0x6E7B8B);
        t.palette.accent = Rgb(0xC0432B);  t.palette.background = Rgb(0x0B0B0C);
        t.glow = 0.14F; t.bloom = 0.18F; t.motion = 1.00F; t.noise = 0.28F;
        t.distortion = 0.85F; t.transitionSpeed = 7.0F;
        add(std::move(t));
    }
    {   // F4 — Neon Rave: saturated green / pink / yellow, maximum energy.
        Theme t;
        t.id = "neon-rave"; t.name = "Neon Rave";
        t.palette.primary = Rgb(0x39FF14); t.palette.secondary = Rgb(0xFF00A0);
        t.palette.accent = Rgb(0xFFF400);  t.palette.background = Rgb(0x0A0014);
        t.glow = 0.60F; t.bloom = 0.70F; t.motion = 1.40F; t.noise = 0.08F;
        t.distortion = 1.30F; t.transitionSpeed = 10.0F;
        add(std::move(t));
    }
    {   // F5 — Aurora: teal and violet curtains, smooth and flowing.
        Theme t;
        t.id = "aurora"; t.name = "Aurora";
        t.palette.primary = Rgb(0x2AF5D0); t.palette.secondary = Rgb(0x1E6091);
        t.palette.accent = Rgb(0xB5FFE1);  t.palette.background = Rgb(0x02040A);
        t.glow = 0.40F; t.bloom = 0.45F; t.motion = 0.70F; t.noise = 0.03F;
        t.distortion = 1.50F; t.transitionSpeed = 4.0F;
        add(std::move(t));
    }
    {   // F6 — Nebula: purple and gold clouds, dreamy and slow.
        Theme t;
        t.id = "nebula"; t.name = "Nebula";
        t.palette.primary = Rgb(0xB24BF3); t.palette.secondary = Rgb(0x3B1E6D);
        t.palette.accent = Rgb(0xFFC857);  t.palette.background = Rgb(0x0A0417);
        t.glow = 0.50F; t.bloom = 0.60F; t.motion = 0.60F; t.noise = 0.05F;
        t.distortion = 1.60F; t.transitionSpeed = 4.0F;
        add(std::move(t));
    }
    {   // F7 — Matrix: green digital rain on near-black.
        Theme t;
        t.id = "matrix"; t.name = "Matrix";
        t.palette.primary = Rgb(0x00FF41); t.palette.secondary = Rgb(0x008F11);
        t.palette.accent = Rgb(0xCCFFCC);  t.palette.background = Rgb(0x000500);
        t.glow = 0.35F; t.bloom = 0.40F; t.motion = 1.20F; t.noise = 0.30F;
        t.distortion = 0.80F; t.transitionSpeed = 9.0F;
        add(std::move(t));
    }

    currentIndex_ = 0; // Cyberpunk
}

std::size_t ThemeManager::Upsert(Theme theme, std::string sourcePath) {
    for (std::size_t i = 0; i < themes_.size(); ++i) {
        if (themes_[i].id == theme.id) {
            themes_[i]      = std::move(theme);
            sourcePaths_[i] = std::move(sourcePath);
            return i;
        }
    }
    themes_.push_back(std::move(theme));
    sourcePaths_.push_back(std::move(sourcePath));
    return themes_.size() - 1;
}

bool ThemeManager::LoadTheme(const std::string& path, std::string* error) {
    std::string text;
    if (!ReadFile(path, text)) {
        if (error != nullptr) {
            *error = "cannot open theme file: " + path;
        }
        return false;
    }
    Theme theme;
    if (!FromJson(text, theme, error)) {
        return false;
    }
    if (theme.id.empty()) {
        if (error != nullptr) {
            *error = "theme is missing an \"id\"";
        }
        return false;
    }
    currentIndex_ = Upsert(std::move(theme), path);
    return true;
}

std::size_t ThemeManager::LoadThemesFromDirectory(const std::string& dir) {
    namespace fs = std::filesystem;
    std::error_code ec;
    if (!fs::is_directory(dir, ec)) {
        return 0;
    }

    // Collect and sort so directory order does not affect load determinism.
    std::vector<std::string> files;
    for (const auto& entry : fs::directory_iterator(dir, ec)) {
        if (ec) {
            break;
        }
        if (entry.is_regular_file() && entry.path().extension() == ".json") {
            files.push_back(entry.path().string());
        }
    }
    std::sort(files.begin(), files.end());

    std::size_t loaded = 0;
    for (const std::string& file : files) {
        std::string text;
        if (!ReadFile(file, text)) {
            continue;
        }
        Theme theme;
        std::string err;
        if (!FromJson(text, theme, &err) || theme.id.empty()) {
            continue;
        }
        Upsert(std::move(theme), file);
        ++loaded;
    }
    return loaded;
}

bool ThemeManager::SetTheme(const std::string_view id) noexcept {
    for (std::size_t i = 0; i < themes_.size(); ++i) {
        if (themes_[i].id == id) {
            currentIndex_ = i;
            return true;
        }
    }
    return false;
}

bool ThemeManager::SetThemeByIndex(const std::size_t index) noexcept {
    if (index >= themes_.size()) {
        return false;
    }
    currentIndex_ = index;
    return true;
}

const Theme& ThemeManager::CurrentTheme() const noexcept {
    return themes_[currentIndex_];
}

std::string_view ThemeManager::CurrentId() const noexcept {
    return themes_[currentIndex_].id;
}

bool ThemeManager::ReloadTheme(std::string* error) {
    const std::string& path = sourcePaths_[currentIndex_];
    if (path.empty()) {
        if (error != nullptr) {
            *error = "current theme is a built-in with no source file to reload";
        }
        return false;
    }
    std::string text;
    if (!ReadFile(path, text)) {
        if (error != nullptr) {
            *error = "cannot open theme file: " + path;
        }
        return false;
    }
    Theme theme;
    if (!FromJson(text, theme, error)) {
        return false;
    }
    themes_[currentIndex_] = std::move(theme);
    return true;
}

void ThemeManager::NextTheme() noexcept {
    if (!themes_.empty()) {
        currentIndex_ = (currentIndex_ + 1) % themes_.size();
    }
}

void ThemeManager::PreviousTheme() noexcept {
    if (!themes_.empty()) {
        currentIndex_ = (currentIndex_ + themes_.size() - 1) % themes_.size();
    }
}

bool ThemeManager::SaveTheme(const Theme& theme, const std::string& path) {
    std::ofstream file(path, std::ios::binary);
    if (!file) {
        return false;
    }
    file << ToJson(theme);
    return static_cast<bool>(file);
}

} // namespace papagedon::visual
