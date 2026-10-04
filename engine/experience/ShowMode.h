#pragma once

#include <string_view>

namespace papagedon {

enum class ShowMode { Open, Trance, Badman };

constexpr ShowMode ShowModeForTheme(std::string_view id) noexcept {
    return id == "trance" ? ShowMode::Trance
         : id == "badman" ? ShowMode::Badman : ShowMode::Open;
}

constexpr bool SceneAllowed(ShowMode section, ShowMode scene) noexcept {
    return section == ShowMode::Open || section == scene;
}

} // namespace papagedon
