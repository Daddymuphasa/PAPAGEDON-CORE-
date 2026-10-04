#pragma once

namespace papagedon::audio {

/// Semantic information derived from one analyzed audio frame.
///
/// Normalized values use the [0.0, 1.0] range. A zero BPM or confidence
/// indicates that the analyzer has not produced a meaningful estimate.
struct ExperienceSignals final {
    float energy = 0.0F;
    float intensity = 0.0F;
    
    // Band analysis (normalized 0.0 - 1.0)
    float bass = 0.0F;   // 20 Hz - 250 Hz
    float mid = 0.0F;    // 250 Hz - 4000 Hz
    float treble = 0.0F; // 4000 Hz - 20000 Hz

    // Temporal semantics
    bool beat = false;
    float bpm = 0.0F;
    
    // Legacy support for graph dependencies
    float tension = 0.0F; 

    float confidence = 0.0F;

    [[nodiscard]] bool IsFinite() const noexcept;
};

} // namespace papagedon::audio
