#include "ExperienceGraph.h"

#include <algorithm>
#include <cmath>
#include <chrono>

namespace papagedon {
namespace {

[[nodiscard]] float Clamp01(const float value) noexcept {
    return std::clamp(value, 0.0F, 1.0F);
}

} // namespace

bool ExperienceGraph::Initialize() {
    if (initialized_) {
        return true;
    }

    currentState_ = {};
    currentState_.timestamp = std::chrono::steady_clock::now();
    initialized_ = true;
    return true;
}

ExperienceState ExperienceGraph::Update(const audio::ExperienceSignals& signals) noexcept {
    if (!initialized_) {
        Initialize();
    }

    const float energy = Clamp01(signals.energy);
    const float intensity = Clamp01(signals.intensity);
    const float tension = Clamp01(signals.tension);
    const float transitionPulse = std::clamp(
        (intensity > kHighIntensityThreshold ? kTransitionProgressStep : 0.0F) +
            (currentEvent_ == ExperienceEvent::BuildUp ? kTransitionProgressStep * 0.5F : 0.0F),
        0.0F,
        1.0F);

    const float tensionDelta = tension - previousTension_;
    const bool risingTension = tensionDelta > kBuildUpTensionDelta;

    if (energy <= kSilenceEnergyThreshold) {
        silenceElapsedSeconds_ += 1.0F;
    } else {
        silenceElapsedSeconds_ = 0.0F;
    }

    if (energy <= kSilenceEnergyThreshold && silenceElapsedSeconds_ >= kSilenceDurationSeconds) {
        currentEvent_ = ExperienceEvent::Silence;
        buildUpActive_ = false;
        dropActive_ = false;
    } else if (energy <= kLowEnergyThreshold) {
        currentEvent_ = ExperienceEvent::Calm;
        buildUpActive_ = false;
        dropActive_ = false;
    } else if (risingTension && intensity >= kHighIntensityThreshold) {
        currentEvent_ = ExperienceEvent::BuildUp;
        buildUpActive_ = true;
        dropActive_ = false;
    } else if (buildUpActive_ && intensity >= kHighIntensityThreshold && energy >= kHighEnergyThreshold) {
        currentEvent_ = ExperienceEvent::Drop;
        dropActive_ = true;
    } else if (energy >= kHighEnergyThreshold && signals.beat) {
        currentEvent_ = ExperienceEvent::Beat;
        buildUpActive_ = false;
        dropActive_ = false;
    } else {
        currentEvent_ = ExperienceEvent::None;
    }

    currentState_.mood = [&]() {
        switch (currentEvent_) {
        case ExperienceEvent::Beat:
            return "Energetic";
        case ExperienceEvent::BuildUp:
            return "Building";
        case ExperienceEvent::Drop:
            return "Release";
        case ExperienceEvent::Calm:
            return "Calm";
        case ExperienceEvent::Silence:
            return "Silent";
        case ExperienceEvent::Transition:
            return "Transition";
        case ExperienceEvent::None:
            return "Idle";
        }

        return "Idle";
    }();

    currentState_.energy = energy;
    currentState_.intensity = intensity;
    currentState_.tension = tension;
    currentState_.transitionProgress = Clamp01(currentState_.transitionProgress + transitionPulse);
    currentState_.activeScene = currentState_.mood;
    currentState_.timestamp = std::chrono::steady_clock::now();
    currentState_.event = currentEvent_;

    previousTension_ = tension;
    return currentState_;
}

const ExperienceState& ExperienceGraph::GetCurrentState() const noexcept {
    return currentState_;
}

ExperienceEvent ExperienceGraph::GetCurrentEvent() const noexcept {
    return currentEvent_;
}

} // namespace papagedon
