#include "AudioAnalyzer.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <numeric>

namespace papagedon::audio {

constexpr size_t kFftSize = 1024;
constexpr size_t kBeatHistorySize = 43; // ~1 second at 43 fps (1024 frames @ 44.1kHz is 23ms)
constexpr float kBeatThresholdMultiplier = 1.4f;

AudioAnalyzer::AudioAnalyzer() {
    fft_ = std::make_unique<SimpleFFT>(kFftSize);
    bassHistory_.resize(kBeatHistorySize, 0.0f);
}

AudioAnalyzer::~AudioAnalyzer() = default;

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
    std::fill(bassHistory_.begin(), bassHistory_.end(), 0.0f);
    bassHistorySum_ = 0.0f;
    bassHistoryIndex_ = 0;
    beatCooldown_ = 0;
}

ExperienceSignals AudioAnalyzer::Analyze(const AudioFrame& frame) noexcept {
    if (!frame.IsValid() || frame.samples.empty()) {
        return latestSignals_; // Return last signals if not enough data
    }

    const size_t numFrames = frame.samples.size() / frame.channelCount;
    if (numFrames == 0) return latestSignals_;

    // 1. Downmix to mono and apply Hann window
    size_t copySize = std::min(numFrames, kFftSize);
    monoBuffer_.resize(kFftSize, 0.0f);
    
    float totalAmplitude = 0.0F;

    for (size_t i = 0; i < copySize; ++i) {
        float mono = 0.0f;
        for (std::uint32_t c = 0; c < frame.channelCount; ++c) {
            mono += frame.samples[i * frame.channelCount + c];
        }
        mono /= static_cast<float>(frame.channelCount);

        totalAmplitude += std::clamp(std::abs(mono), 0.0F, 1.0F);

        // Hann window: 0.5 * (1 - cos(2*pi*n/N))
        const float window = 0.5f * (1.0f - std::cos(2.0f * 3.14159265358979323846f * i / (kFftSize - 1)));
        monoBuffer_[i] = mono * window;
    }

    const float energy = totalAmplitude / static_cast<float>(copySize);

    // 2. Perform FFT
    fft_->Execute(monoBuffer_, fftOutput_);

    // 3. Compute magnitude spectrum and group into bands
    const float nyquist = static_cast<float>(frame.sampleRate) / 2.0f;
    const float binResolution = static_cast<float>(frame.sampleRate) / static_cast<float>(kFftSize);

    float bassEnergy = 0.0f;
    float midEnergy = 0.0f;
    float trebleEnergy = 0.0f;

    size_t bassCount = 0;
    size_t midCount = 0;
    size_t trebleCount = 0;

    for (size_t i = 0; i < kFftSize / 2; ++i) {
        float freq = static_cast<float>(i) * binResolution;
        float magnitude = std::abs(fftOutput_[i]);
        
        // Scale magnitude to a reasonable range
        magnitude = magnitude / (kFftSize / 2.0f);

        if (freq >= 20.0f && freq < 250.0f) {
            bassEnergy += magnitude;
            bassCount++;
        } else if (freq >= 250.0f && freq < 4000.0f) {
            midEnergy += magnitude;
            midCount++;
        } else if (freq >= 4000.0f && freq < 20000.0f) {
            trebleEnergy += magnitude;
            trebleCount++;
        }
    }

    if (bassCount > 0) bassEnergy /= static_cast<float>(bassCount);
    if (midCount > 0) midEnergy /= static_cast<float>(midCount);
    if (trebleCount > 0) trebleEnergy /= static_cast<float>(trebleCount);

    // Normalize to [0, 1] - these multipliers might need tuning
    bassEnergy = std::clamp(bassEnergy * 15.0f, 0.0f, 1.0f);
    midEnergy = std::clamp(midEnergy * 25.0f, 0.0f, 1.0f);
    trebleEnergy = std::clamp(trebleEnergy * 40.0f, 0.0f, 1.0f);

    // 4. Beat Detection
    bool beat = false;
    if (beatCooldown_ > 0) {
        beatCooldown_--;
    } else {
        float averageBass = bassHistorySum_ / kBeatHistorySize;
        if (bassEnergy > averageBass * kBeatThresholdMultiplier && bassEnergy > 0.15f) {
            beat = true;
            beatCooldown_ = 8; // ~185ms at 43 fps
        }
    }

    // Update history
    bassHistorySum_ -= bassHistory_[bassHistoryIndex_];
    bassHistory_[bassHistoryIndex_] = bassEnergy;
    bassHistorySum_ += bassEnergy;
    bassHistoryIndex_ = (bassHistoryIndex_ + 1) % kBeatHistorySize;

    return {
        .energy = energy,
        .intensity = energy,
        .bass = bassEnergy,
        .mid = midEnergy,
        .treble = trebleEnergy,
        .beat = beat,
        .bpm = 0.0F,
        .tension = energy * 0.5F,
        .confidence = 1.0f
    };
}

} // namespace papagedon::audio
