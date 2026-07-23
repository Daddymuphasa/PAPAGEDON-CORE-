#include "SceneDNA.h"

#include <algorithm>
#include <chrono>

namespace papagedon {
namespace {

using namespace std::chrono_literals;

SceneProfile MakeProfile(
    const char* sceneId,
    const char* name,
    const char* environmentType,
    const char* mood,
    std::vector<std::string> colorPalette,
    const char* lightingStyle,
    const char* weatherType,
    const char* cameraStyle,
    const char* particleStyle,
    const char* atmosphere,
    const std::chrono::milliseconds transitionDuration) {
    return {
        .sceneId = sceneId,
        .name = name,
        .environmentType = environmentType,
        .mood = mood,
        .colorPalette = std::move(colorPalette),
        .lightingStyle = lightingStyle,
        .weatherType = weatherType,
        .cameraStyle = cameraStyle,
        .particleStyle = particleStyle,
        .atmosphere = atmosphere,
        .transitionDuration = transitionDuration,
    };
}

} // namespace

const char* ToString(const ExperienceState state) noexcept {
    switch (state) {
    case ExperienceState::Calm:
        return "Calm";
    case ExperienceState::BuildUp:
        return "BuildUp";
    case ExperienceState::Drop:
        return "Drop";
    case ExperienceState::Silence:
        return "Silence";
    case ExperienceState::Ambient:
        return "Ambient";
    }

    return "Ambient";
}

bool SceneDNA::Initialize() {
    if (initialized_) {
        return true;
    }

    profiles_[static_cast<std::size_t>(ProfileIndex::Calm)] = MakeProfile(
        "calm", "Calm", "Still Water", "Reflective", {"#102A43", "#3E7CB1", "#BFD7EA"},
        "Soft diffuse", "Clear", "Slow drift", "Sparse motes", "Quiet haze", 1200ms);
    profiles_[static_cast<std::size_t>(ProfileIndex::BuildUp)] = MakeProfile(
        "build-up", "BuildUp", "Rising Terrain", "Anticipatory", {"#2B193D", "#7B2CBF", "#FF9F1C"},
        "Growing contrast", "Gathering clouds", "Progressive push", "Ascending sparks", "Charged air", 900ms);
    profiles_[static_cast<std::size_t>(ProfileIndex::Drop)] = MakeProfile(
        "drop", "Drop", "Open Expanse", "Euphoric", {"#120078", "#FE346E", "#FFFFD2"},
        "High impact", "Electric storm", "Dynamic sweep", "Bursting energy", "Vibrant bloom", 450ms);
    profiles_[static_cast<std::size_t>(ProfileIndex::Silence)] = MakeProfile(
        "silence", "Silence", "Empty Horizon", "Suspended", {"#111111", "#545454", "#D9D9D9"},
        "Low key", "Still", "Locked-off", "None", "Near vacuum", 1500ms);
    profiles_[static_cast<std::size_t>(ProfileIndex::Ambient)] = MakeProfile(
        "ambient", "Ambient", "Abstract Space", "Neutral", {"#1F2933", "#627D98", "#D9E2EC"},
        "Balanced", "Gentle mist", "Observational", "Subtle dust", "Soft depth", 1000ms);

    const auto now = std::chrono::steady_clock::now();
    currentScene_ = {
        .activeProfile = &ProfileFor(ExperienceState::Ambient),
        .blendFactor = 1.0F,
        .timestamp = now,
    };
    previousProfile_ = currentScene_.activeProfile;
    initialized_ = true;
    return true;
}

void SceneDNA::Update(const ExperienceState& experienceState) noexcept {
    if (!initialized_) {
        return;
    }

    const SceneProfile& targetProfile = ProfileFor(experienceState);
    if (currentScene_.activeProfile != &targetProfile) {
        RefreshTransition();
        previousProfile_ = currentScene_.activeProfile;
        currentScene_.activeProfile = &targetProfile;
        currentScene_.blendFactor = 0.0F;
        currentScene_.timestamp = std::chrono::steady_clock::now();
    }

    RefreshTransition();
}

void SceneDNA::Shutdown() noexcept {
    initialized_ = false;
    currentScene_ = {};
    previousProfile_ = nullptr;
}

const SceneState& SceneDNA::GetCurrentScene() const noexcept {
    return currentScene_;
}

const SceneProfile& SceneDNA::ProfileFor(const ExperienceState state) const noexcept {
    switch (state) {
    case ExperienceState::Calm:
        return profiles_[static_cast<std::size_t>(ProfileIndex::Calm)];
    case ExperienceState::BuildUp:
        return profiles_[static_cast<std::size_t>(ProfileIndex::BuildUp)];
    case ExperienceState::Drop:
        return profiles_[static_cast<std::size_t>(ProfileIndex::Drop)];
    case ExperienceState::Silence:
        return profiles_[static_cast<std::size_t>(ProfileIndex::Silence)];
    case ExperienceState::Ambient:
        return profiles_[static_cast<std::size_t>(ProfileIndex::Ambient)];
    }

    return profiles_[static_cast<std::size_t>(ProfileIndex::Ambient)];
}

void SceneDNA::RefreshTransition() noexcept {
    if (currentScene_.activeProfile == nullptr || currentScene_.blendFactor >= 1.0F) {
        return;
    }

    const auto duration = currentScene_.activeProfile->transitionDuration;
    if (duration.count() <= 0) {
        currentScene_.blendFactor = 1.0F;
        return;
    }

    const auto elapsed = std::chrono::steady_clock::now() - currentScene_.timestamp;
    const float progress = std::chrono::duration<float>(elapsed).count() /
        std::chrono::duration<float>(duration).count();
    currentScene_.blendFactor = std::clamp(progress, 0.0F, 1.0F);
}

} // namespace papagedon
