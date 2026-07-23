#pragma once

#include "SceneState.h"

#include <array>

namespace papagedon {

/// High-level experience decisions supplied by the Experience Graph.
enum class ExperienceState {
    Calm,
    BuildUp,
    Drop,
    Silence,
    Ambient,
};

[[nodiscard]] const char* ToString(ExperienceState state) noexcept;

/// Converts experience decisions into data-only scene state.
class SceneDNA final {
public:
    bool Initialize();
    void Update(const ExperienceState& experienceState) noexcept;
    void Shutdown() noexcept;

    [[nodiscard]] const SceneState& GetCurrentScene() const noexcept;

private:
    enum class ProfileIndex : std::size_t {
        Calm,
        BuildUp,
        Drop,
        Silence,
        Ambient,
    };

    [[nodiscard]] const SceneProfile& ProfileFor(ExperienceState state) const noexcept;
    void RefreshTransition() noexcept;

    std::array<SceneProfile, 5> profiles_{};
    SceneState currentScene_{};
    const SceneProfile* previousProfile_ = nullptr;
    bool initialized_ = false;
};

} // namespace papagedon
