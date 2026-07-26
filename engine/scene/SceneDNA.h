#pragma once

#include "ExperienceState.h"
#include "SceneState.h"

#include <array>

namespace papagedon {

// Forward-declare to avoid pulling in the experience module's audio dependency.
struct ExperienceGraphOutput;

/// Converts experience decisions into data-only scene state.
class SceneDNA final {
public:
    bool Initialize();

    /// Updates the current scene from Experience Graph output.
    /// Copies energy, intensity and mood into SceneState so the Renderer
    /// can consume them without knowing about audio or experience types.
    void Update(const ExperienceGraphOutput& graphOutput) noexcept;

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
