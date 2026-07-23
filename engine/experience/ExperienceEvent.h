#pragma once

#include <cstdint>
#include <string_view>

namespace papagedon {

enum class ExperienceEvent : std::uint8_t {
    None,
    Beat,
    BuildUp,
    Drop,
    Calm,
    Silence,
    Transition,
};

[[nodiscard]] constexpr std::string_view ToString(const ExperienceEvent event) noexcept {
    switch (event) {
    case ExperienceEvent::None:
        return "None";
    case ExperienceEvent::Beat:
        return "Beat";
    case ExperienceEvent::BuildUp:
        return "BuildUp";
    case ExperienceEvent::Drop:
        return "Drop";
    case ExperienceEvent::Calm:
        return "Calm";
    case ExperienceEvent::Silence:
        return "Silence";
    case ExperienceEvent::Transition:
        return "Transition";
    }

    return "Unknown";
}

} // namespace papagedon
