#include "AudioAnalyzer.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <numeric>

namespace papagedon::audio {

constexpr size_t kFftSize = 1024;
constexpr size_t kBeatHistorySize = 43; // adaptive-threshold window (bass energy samples)
constexpr float kBeatThresholdMultiplier = 1.4f;
constexpr float kBeatOnsetThreshold      = 0.12f; // min bass rise/frame for an onset

// Tempo tracking (all in seconds on the audio playback timeline).
constexpr double kMinBeatIntervalSeconds = 0.15;  // debounce; rejects <=400 BPM doubles
constexpr double kTempoMinInterval       = 0.30;  // 200 BPM ceiling for the histogram
constexpr double kTempoMaxInterval       = 1.50;  // 40  BPM floor   for the histogram
constexpr size_t kTempoHistorySize       = 8;     // recent intervals kept for the median

// Adaptive normalization: scale a raw band magnitude to its slowly-decaying peak
// so the output fills 0..1 and swings with the music's dynamics regardless of
// input gain. A silence gate stops it amplifying noise between tracks.
float AdaptiveNorm(float raw, float& peak) noexcept {
    peak = std::max(raw, peak * 0.9992f);       // envelope follower, slow release
    if (raw < 1.0e-4f) {
        return 0.0f;                            // near-silence
    }
    const float n = raw / std::max(peak, 1.0e-4f);
    // Gentle upward curve so quieter mid/high detail still reads on screen.
    return std::clamp(std::pow(n, 0.75f), 0.0f, 1.0f);
}

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
    previousBass_ = 0.0f;
    lastBeatTimeSeconds_ = -1.0;
    beatBpmHistory_.clear();
    beatBpmIndex_ = 0;
    currentBpm_ = 0.0f;
    bassPeak_ = 0.0f;
    midPeak_ = 0.0f;
    treblePeak_ = 0.0f;
    energyPeak_ = 0.0f;
    intensityPeak_ = 0.0f;
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

    // Keep the raw band magnitudes for adaptive normalization of the output.
    const float rawBass   = bassEnergy;
    const float rawMid    = midEnergy;
    const float rawTreble = trebleEnergy;

    // Fixed-scaled bass drives the beat detector (keeps its threshold behaviour).
    bassEnergy = std::clamp(bassEnergy * 15.0f, 0.0f, 1.0f);

    // 4. Beat detection — a bass spike, detected two complementary ways so kicks
    //    still register when they ride on top of a sustained bassline:
    //      * level:  bass energy rises clearly above its recent adaptive average.
    //      * onset:  a sharp positive jump in bass energy since the last frame.
    //    Either one arms a beat candidate (gated by time below).
    const float averageBass = bassHistorySum_ / static_cast<float>(kBeatHistorySize);
    const float bassFlux    = bassEnergy - previousBass_;
    previousBass_ = bassEnergy;

    const bool levelSpike = bassEnergy > averageBass * kBeatThresholdMultiplier &&
                            bassEnergy > 0.15f;
    const bool onsetSpike = bassFlux > kBeatOnsetThreshold && bassEnergy > 0.22f;
    const bool bassSpike  = levelSpike || onsetSpike;

    // Update the rolling bass history feeding the adaptive threshold above.
    bassHistorySum_ -= bassHistory_[bassHistoryIndex_];
    bassHistory_[bassHistoryIndex_] = bassEnergy;
    bassHistorySum_ += bassEnergy;
    bassHistoryIndex_ = (bassHistoryIndex_ + 1) % kBeatHistorySize;

    // 5. Time-based gating + tempo estimation, clocked by the audio timeline so
    //    results do not change with the render frame rate.
    const double now = frame.timestampSeconds;
    if (now < lastBeatTimeSeconds_) {
        // Playback jumped backwards (seek/restart) — drop stale tempo state.
        lastBeatTimeSeconds_ = -1.0;
    }

    bool beat = false;
    if (bassSpike) {
        const bool firstBeat = lastBeatTimeSeconds_ < 0.0;
        const double sinceLast = now - lastBeatTimeSeconds_;
        if (firstBeat || sinceLast >= kMinBeatIntervalSeconds) {
            beat = true;
            if (!firstBeat) {
                UpdateTempo(sinceLast);
            }
            lastBeatTimeSeconds_ = now;
        }
    }

    // Adaptive, full-range output — this is what makes the visuals swing with
    // the music instead of sitting flat.
    const float outBass      = AdaptiveNorm(rawBass, bassPeak_);
    const float outMid       = AdaptiveNorm(rawMid, midPeak_);
    const float outTreble    = AdaptiveNorm(rawTreble, treblePeak_);
    const float outEnergy    = AdaptiveNorm(energy, energyPeak_);
    const float outIntensity = AdaptiveNorm(rawMid + rawTreble, intensityPeak_);

    return {
        .energy = outEnergy,
        .intensity = outIntensity,
        .bass = outBass,
        .mid = outMid,
        .treble = outTreble,
        .beat = beat,
        .bpm = currentBpm_,
        .tension = outEnergy * 0.5F,
        .confidence = 1.0f
    };
}

void AudioAnalyzer::UpdateTempo(const double intervalSeconds) noexcept {
    // Ignore implausible gaps — usually a missed beat (too long) or a
    // double-triggered transient (too short) rather than a real tempo change.
    if (intervalSeconds < kTempoMinInterval || intervalSeconds > kTempoMaxInterval) {
        return;
    }

    const float bpm = static_cast<float>(60.0 / intervalSeconds);
    if (beatBpmHistory_.size() < kTempoHistorySize) {
        beatBpmHistory_.push_back(bpm);
    } else {
        beatBpmHistory_[beatBpmIndex_] = bpm;
        beatBpmIndex_ = (beatBpmIndex_ + 1) % kTempoHistorySize;
    }

    // Median of recent estimates — robust to the occasional missed/extra beat.
    std::vector<float> sorted(beatBpmHistory_);
    std::sort(sorted.begin(), sorted.end());
    currentBpm_ = sorted[sorted.size() / 2];
}

} // namespace papagedon::audio
