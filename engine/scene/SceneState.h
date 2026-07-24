#pragma once

#include "SceneProfile.h"

#include <chrono>

namespace papagedon {

/// The current scene selection and its transition progress.
struct SceneState final {
    const SceneProfile* activeProfile = nullptr;
    const SceneProfile* previousProfile = nullptr;
    float transitionProgress = 1.0F;
    std::chrono::steady_clock::time_point timestamp{};
};

} // namespace papagedon
