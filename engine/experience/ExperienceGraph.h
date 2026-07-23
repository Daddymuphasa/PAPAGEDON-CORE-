#pragma once

#include <chrono>

#include "ExperienceState.h"

#include <ExperienceSignals.h>

namespace papagedon {

class ExperienceGraph final {
public:
    ExperienceGraph() = default;

    ExperienceGraph(const ExperienceGraph&) = delete;
    ExperienceGraph& operator=(const ExperienceGraph&) = delete;

    bool Initialize();

    [[nodiscard]] ExperienceState Update(const audio::ExperienceSignals& signals) noexcept;
    [[nodiscard]] const ExperienceState& GetCurrentState() const noexcept;
    [[nodiscard]] ExperienceEvent GetCurrentEvent() const noexcept;

private:
    static constexpr float kHighEnergyThreshold = 0.55F;
    static constexpr float kHighIntensityThreshold = 0.65F;
    static constexpr float kLowEnergyThreshold = 0.20F;
    static constexpr float kSilenceEnergyThreshold = 0.05F;
    static constexpr float kBuildUpTensionDelta = 0.03F;
    static constexpr float kSilenceDurationSeconds = 2.0F;
    static constexpr float kTransitionProgressStep = 0.10F;

    bool initialized_ = false;
    ExperienceState currentState_{};
    ExperienceEvent currentEvent_ = ExperienceEvent::None;
    float previousTension_ = 0.0F;
    float silenceElapsedSeconds_ = 0.0F;
    bool buildUpActive_ = false;
    bool dropActive_ = false;
};

} // namespace papagedon
