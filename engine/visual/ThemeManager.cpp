#include "ThemeManager.h"

#include <fstream>
#include <sstream>
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
// Built-in themes
//
// Six distinct visual identities.  Each maps its palette across the shader's
// dark → bright ramp (secondary → primary → accent) with `background` filling the
// darkest regions, and tunes contrast / glow / bloom for its mood.  The style
// axes are carried as data for the renderer and future subsystems.
// ──────────────────────────────────────────────────────────────────────────────
void ThemeManager::RegisterBuiltins() {
    themes_.clear();
    themes_.reserve(6);

    {   // Cyberpunk — electric cyan and magenta over deep indigo.
        Theme t;
        t.id   = "cyberpunk";
        t.name = "Cyberpunk";
        t.palette.primary    = Rgb(0x00E5FF);
        t.palette.secondary  = Rgb(0x7B2CBF);
        t.palette.accent     = Rgb(0xFF2A6D);
        t.palette.background = Rgb(0x05010F);
        t.contrast      = 1.25F;
        t.glowStrength  = 0.45F;
        t.bloomStrength = 0.55F;
        t.particleStyle   = ParticleStyle::Sparks;
        t.motionStyle     = MotionStyle::Pulsing;
        t.geometryStyle   = GeometryStyle::Grid;
        t.noiseStyle      = NoiseStyle::Digital;
        t.transitionStyle = TransitionStyle::Glitch;
        themes_.push_back(std::move(t));
    }
    {   // Neon Rave — saturated green / pink / yellow, maximum energy.
        Theme t;
        t.id   = "neon-rave";
        t.name = "Neon Rave";
        t.palette.primary    = Rgb(0x39FF14);
        t.palette.secondary  = Rgb(0xFF00A0);
        t.palette.accent     = Rgb(0xFFF400);
        t.palette.background = Rgb(0x0A0014);
        t.contrast      = 1.35F;
        t.glowStrength  = 0.6F;
        t.bloomStrength = 0.7F;
        t.particleStyle   = ParticleStyle::Confetti;
        t.motionStyle     = MotionStyle::Strobing;
        t.geometryStyle   = GeometryStyle::Radial;
        t.noiseStyle      = NoiseStyle::Digital;
        t.transitionStyle = TransitionStyle::Cut;
        themes_.push_back(std::move(t));
    }
    {   // Minimal — restrained monochrome with a single cool accent.
        Theme t;
        t.id   = "minimal";
        t.name = "Minimal";
        t.palette.primary    = Rgb(0xF5F5F5);
        t.palette.secondary  = Rgb(0x9AA5B1);
        t.palette.accent     = Rgb(0x6BA8FF);
        t.palette.background = Rgb(0x0E0E10);
        t.contrast      = 0.9F;
        t.glowStrength  = 0.08F;
        t.bloomStrength = 0.12F;
        t.particleStyle   = ParticleStyle::None;
        t.motionStyle     = MotionStyle::Smooth;
        t.geometryStyle   = GeometryStyle::Organic;
        t.noiseStyle      = NoiseStyle::None;
        t.transitionStyle = TransitionStyle::Fade;
        themes_.push_back(std::move(t));
    }
    {   // Industrial — steel and amber, hard and grainy.
        Theme t;
        t.id   = "industrial";
        t.name = "Industrial";
        t.palette.primary    = Rgb(0xFFB000);
        t.palette.secondary  = Rgb(0x6E7B8B);
        t.palette.accent     = Rgb(0xC0432B);
        t.palette.background = Rgb(0x0B0B0C);
        t.contrast      = 1.3F;
        t.glowStrength  = 0.12F;
        t.bloomStrength = 0.18F;
        t.particleStyle   = ParticleStyle::Embers;
        t.motionStyle     = MotionStyle::Aggressive;
        t.geometryStyle   = GeometryStyle::Grid;
        t.noiseStyle      = NoiseStyle::Film;
        t.transitionStyle = TransitionStyle::Wipe;
        themes_.push_back(std::move(t));
    }
    {   // Psychedelic — swirling rainbow, soft and hypnotic.
        Theme t;
        t.id   = "psychedelic";
        t.name = "Psychedelic";
        t.palette.primary    = Rgb(0xFF3CAC);
        t.palette.secondary  = Rgb(0x2AF5D0);
        t.palette.accent     = Rgb(0xFFD23F);
        t.palette.background = Rgb(0x12002E);
        t.contrast      = 1.05F;
        t.glowStrength  = 0.5F;
        t.bloomStrength = 0.6F;
        t.particleStyle   = ParticleStyle::Bokeh;
        t.motionStyle     = MotionStyle::Hypnotic;
        t.geometryStyle   = GeometryStyle::Fractal;
        t.noiseStyle      = NoiseStyle::Turbulent;
        t.transitionStyle = TransitionStyle::Dissolve;
        themes_.push_back(std::move(t));
    }
    {   // Dark Techno — cold blues and a blood-red accent, moody and low-key.
        Theme t;
        t.id   = "dark-techno";
        t.name = "Dark Techno";
        t.palette.primary    = Rgb(0xAEB6C2);
        t.palette.secondary  = Rgb(0x1B2A4A);
        t.palette.accent     = Rgb(0xE23B3B);
        t.palette.background = Rgb(0x040406);
        t.contrast      = 1.3F;
        t.glowStrength  = 0.25F;
        t.bloomStrength = 0.22F;
        t.particleStyle   = ParticleStyle::Dust;
        t.motionStyle     = MotionStyle::Pulsing;
        t.geometryStyle   = GeometryStyle::Tunnel;
        t.noiseStyle      = NoiseStyle::Scanline;
        t.transitionStyle = TransitionStyle::Cut;
        themes_.push_back(std::move(t));
    }

    currentIndex_ = 0; // Cyberpunk
    currentSourcePath_.clear();
}

std::size_t ThemeManager::Upsert(Theme theme) {
    for (std::size_t i = 0; i < themes_.size(); ++i) {
        if (themes_[i].id == theme.id) {
            themes_[i] = std::move(theme);
            return i;
        }
    }
    themes_.push_back(std::move(theme));
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
    currentIndex_      = Upsert(std::move(theme));
    currentSourcePath_ = path;
    return true;
}

bool ThemeManager::SetTheme(const std::string_view id) noexcept {
    for (std::size_t i = 0; i < themes_.size(); ++i) {
        if (themes_[i].id == id) {
            currentIndex_ = i;
            currentSourcePath_.clear();
            return true;
        }
    }
    return false;
}

const Theme& ThemeManager::CurrentTheme() const noexcept {
    return themes_[currentIndex_];
}

std::string_view ThemeManager::CurrentId() const noexcept {
    return themes_[currentIndex_].id;
}

bool ThemeManager::ReloadTheme(std::string* error) {
    if (currentSourcePath_.empty()) {
        if (error != nullptr) {
            *error = "current theme has no source file to reload";
        }
        return false;
    }
    std::string text;
    if (!ReadFile(currentSourcePath_, text)) {
        if (error != nullptr) {
            *error = "cannot open theme file: " + currentSourcePath_;
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
        currentSourcePath_.clear();
    }
}

void ThemeManager::PreviousTheme() noexcept {
    if (!themes_.empty()) {
        currentIndex_ = (currentIndex_ + themes_.size() - 1) % themes_.size();
        currentSourcePath_.clear();
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
