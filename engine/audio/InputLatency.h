#pragma once

#include <algorithm>
#include <cmath>

namespace papagedon::audio {

struct CaptureTiming {
    float periodMs = 0.0f;
    float jitterMs = 0.0f;
    float ageMs = 0.0f;
    unsigned observations = 0;
};

// Passive input-pipeline estimate. Speaker, converter and display delay require
// an external reference and are deliberately excluded from this estimate.
class InputLatency final {
public:
    void Update(const CaptureTiming& timing, float sampleRate, float dt) noexcept {
        if (timing.observations == 0 || sampleRate <= 0.0f) { Reset(); return; }
        const float target = timing.periodMs + timing.ageMs + 512000.0f / sampleRate;
        const float alpha = ready_ ? 1.0f - std::exp(-std::clamp(dt, 0.0f, 1.0f) * 3.0f) : 1.0f;
        estimatedMs_ += (std::clamp(target, 0.0f, 500.0f) - estimatedMs_) * alpha;
        confidence_ = std::min(timing.observations / 128.0f, 1.0f) /
            (1.0f + timing.jitterMs / std::max(timing.periodMs, 1.0f));
        stalled_ = timing.ageMs > std::max(100.0f, timing.periodMs * 4.0f);
        ready_ = true;
    }
    void Reset() noexcept { estimatedMs_ = confidence_ = 0.0f; ready_ = stalled_ = false; }
    float EstimatedMs() const noexcept { return estimatedMs_; }
    float Confidence() const noexcept { return confidence_; }
    bool Stalled() const noexcept { return stalled_; }
    // Spend only the remaining 25 ms response budget on visual attack smoothing.
    float AttackRate() const noexcept {
        return ready_ ? 1000.0f / std::clamp(25.0f - estimatedMs_, 2.0f, 10.0f) : 100.0f;
    }
private:
    float estimatedMs_ = 0.0f;
    float confidence_ = 0.0f;
    bool ready_ = false;
    bool stalled_ = false;
};

} // namespace papagedon::audio
