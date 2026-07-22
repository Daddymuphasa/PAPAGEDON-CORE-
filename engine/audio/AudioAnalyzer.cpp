#include "AudioAnalyzer.h"

#include <algorithm>
#include <cmath>
#include <cstddef>

namespace papagedon::audio {

ExperienceSignals AudioAnalyzer::Update(const AudioFrame& frame) noexcept {
    latestSignals_ = Analyze(frame);
    return latestSignals_;
}

ExperienceSignals AudioAnalyzer::Update(
    const std::span<const float> samples,
    const std::uint32_t sampleRate,
    const std::uint32_t channelCount) noexcept {
    return Update(AudioFrame{samples, sampleRate, channelCount});
}

const ExperienceSignals& AudioAnalyzer::LatestSignals() const noexcept {
    return latestSignals_;
}

void AudioAnalyzer::Reset() noexcept {
    latestSignals_ = {};
}

ExperienceSignals AudioAnalyzer::Analyze(const AudioFrame& frame) const noexcept {
    if (!frame.IsValid() || frame.samples.empty()) {
        return {};
    }

    float totalAmplitude = 0.0F;
    for (const float sample : frame.samples) {
        if (!std::isfinite(sample)) {
            return {};
        }

        totalAmplitude += std::clamp(std::abs(sample), 0.0F, 1.0F);
    }

    const float energy = totalAmplitude / static_cast<float>(frame.samples.size());
    return {
        .energy = energy,
        .intensity = energy,
        .tension = energy * 0.5F,
        .beat = false,
        .bpm = 0.0F,
        .confidence = std::min(
            1.0F,
            static_cast<float>(frame.samples.size()) / 1'024.0F),
    };
}

} // namespace papagedon::audio
