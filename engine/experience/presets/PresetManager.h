#pragma once

#include "ExperiencePreset.h"

namespace papagedon {

// ──────────────────────────────────────────────────────────────────────────────
// PresetManager
//
// Owns the single active preset selection.  It holds no preset data of its own —
// only an id indexing the immutable table in ExperiencePreset.cpp — so every
// operation is a trivial index change with no heap allocation.
//
// The Runtime owns one PresetManager and hands CurrentPreset() to the Renderer
// each frame.  Switching presets is therefore instant and requires no restart.
// ──────────────────────────────────────────────────────────────────────────────
class PresetManager final {
public:
    /// Starts on the default preset (Aurora).
    PresetManager() noexcept { LoadDefault(); }

    /// Selects a specific preset.  Out-of-range ids are ignored.
    void SetPreset(PresetId id) noexcept;

    /// Advances to the next preset, wrapping past the last back to the first.
    void NextPreset() noexcept;

    /// Steps to the previous preset, wrapping past the first to the last.
    void PreviousPreset() noexcept;

    /// The currently active preset.  Always valid.
    [[nodiscard]] const ExperiencePreset& CurrentPreset() const noexcept;

    /// The id of the active preset.
    [[nodiscard]] PresetId CurrentId() const noexcept { return activeId_; }

    /// Resets the selection to the default preset (Aurora).
    void LoadDefault() noexcept;

private:
    PresetId activeId_ = PresetId::Aurora;
};

} // namespace papagedon
