#include "PresetManager.h"

namespace papagedon {

void PresetManager::SetPreset(const PresetId id) noexcept {
    if (static_cast<std::size_t>(id) < kPresetCount) {
        activeId_ = id;
    }
}

void PresetManager::NextPreset() noexcept {
    const auto index = static_cast<std::size_t>(activeId_);
    activeId_ = static_cast<PresetId>((index + 1u) % kPresetCount);
}

void PresetManager::PreviousPreset() noexcept {
    const auto index = static_cast<std::size_t>(activeId_);
    activeId_ = static_cast<PresetId>((index + kPresetCount - 1u) % kPresetCount);
}

const ExperiencePreset& PresetManager::CurrentPreset() const noexcept {
    return GetPreset(activeId_);
}

void PresetManager::LoadDefault() noexcept {
    activeId_ = PresetId::Aurora;
}

} // namespace papagedon
