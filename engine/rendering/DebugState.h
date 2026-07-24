#pragma once

namespace papagedon {

/// Real-time engine state for the debug overlay
struct DebugState final {
    float bpm = 0.0F;
    float energy = 0.0F;
    float intensity = 0.0F;
    const char* currentExperience = "";
    float transitionProgress = 0.0F;
};

} // namespace papagedon
