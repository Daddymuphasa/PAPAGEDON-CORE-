#pragma once

#include <chrono>
#include <string>

#include "ExperienceEvent.h"

namespace papagedon {

/// High-level state emitted by the ExperienceGraph.
struct ExperienceState final {
    std::string mood = "Idle";
    float intensity = 0.0F;
    float tension = 0.0F;
    float energy = 0.0F;
    float transitionProgress = 0.0F;
    std::string activeScene = "Idle";
    std::chrono::steady_clock::time_point timestamp{};
    ExperienceEvent event = ExperienceEvent::None;
};

} // namespace papagedon
