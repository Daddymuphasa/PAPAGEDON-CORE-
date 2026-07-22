#pragma once

#include <vector>

namespace papagedon::audio {

/// Semantic information derived from one analyzed audio frame.
///
/// Normalized values use the [0.0, 1.0] range. A zero BPM or confidence
/// indicates that the analyzer has not produced a meaningful estimate.
struct ExperienceSignals final {
    float energy = 0.0F;
    float intensity = 0.0F;
    float tension = 0.0F;
    bool beat = false;
    float bpm = 0.0F;
    float confidence = 0.0F;
    std::vector<float> spectrum{};
    float bassEnergy = 0.0F;
    float midEnergy = 0.0F;
    float trebleEnergy = 0.0F;

    [[nodiscard]] bool IsFinite() const noexcept;
};

} // namespace papagedon::audio
