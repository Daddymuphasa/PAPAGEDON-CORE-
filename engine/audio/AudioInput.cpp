#include "AudioInput.h"

#include <algorithm>
#include <limits>
#include <span>
#include <utility>

#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>

namespace papagedon::audio {
namespace {

[[nodiscard]] bool IsValidStereoOrMonoConfiguration(
    const AudioInput::Config& config) noexcept {
    return config.sampleRate > 0 &&
           (config.channelCount == 1 || config.channelCount == 2) &&
           config.framesPerPeriod > 0;
}

} // namespace

bool AudioInput::Initialize() {
    return Initialize(Config{});
}

bool AudioInput::Initialize(const Config& config) {
    Stop();

    if (!IsValidStereoOrMonoConfiguration(config)) {
        return false;
    }

    config_ = config;
    ResetState();

    if (config_.source == SourceType::File) {
        return InitializeFileSource();
    }

    return InitializeMicrophone();
}

bool AudioInput::Start() {
    if (!initialized_ || running_.load(std::memory_order_acquire)) {
        return false;
    }

    if (config_.source == SourceType::File) {
        running_.store(true, std::memory_order_release);
        return true;
    }

    if (device_ == nullptr) {
        return false;
    }

    const ma_result result = ma_device_start(device_);
    if (result != MA_SUCCESS) {
        running_.store(false, std::memory_order_release);
        return false;
    }

    running_.store(true, std::memory_order_release);
    return true;
}

void AudioInput::Stop() noexcept {
    running_.store(false, std::memory_order_release);

    if (device_ != nullptr) {
        ma_device_stop(device_);
        ma_device_uninit(device_);
        delete device_;
        device_ = nullptr;
    }

    if (decoder_ != nullptr) {
        ma_decoder_uninit(decoder_);
        delete decoder_;
        decoder_ = nullptr;
    }

    std::lock_guard lock{mutex_};
    latestSamples_.clear();
    latestSamples_.shrink_to_fit();
    sampleRate_ = 0;
    channelCount_ = 0;
    frameCount_ = 0;
    initialized_ = false;
    deviceInitialized_ = false;
}

AudioFrame AudioInput::GetLatestSamples() const noexcept {
    std::lock_guard lock{mutex_};
    if (latestSamples_.empty()) {
        return {};
    }

    return AudioFrame{
        .samples = std::span<const float>{latestSamples_},
        .sampleRate = sampleRate_,
        .channelCount = channelCount_,
    };
}

bool AudioInput::IsRunning() const noexcept {
    return running_.load(std::memory_order_acquire);
}

bool AudioInput::InitializeMicrophone() {
    ma_device_config deviceConfig = ma_device_config_init(ma_device_type_capture);
    deviceConfig.capture.format = ma_format_f32;
    deviceConfig.capture.channels = config_.channelCount;
    deviceConfig.capture.shareMode = ma_share_mode_shared;
    deviceConfig.periodSizeInFrames = config_.framesPerPeriod;
    deviceConfig.periods = 2;
    deviceConfig.sampleRate = config_.sampleRate;
    deviceConfig.dataCallback = &AudioInput::CaptureCallback;
    deviceConfig.pUserData = this;

    device_ = new ma_device{};
    const ma_result result = ma_device_init(nullptr, &deviceConfig, device_);
    if (result != MA_SUCCESS) {
        delete device_;
        device_ = nullptr;
        return false;
    }

    initialized_ = true;
    deviceInitialized_ = true;
    return true;
}

bool AudioInput::InitializeFileSource() {
    if (config_.filePath.empty()) {
        return false;
    }

    decoder_ = new ma_decoder{};
    const ma_decoder_config decoderConfig = ma_decoder_config_init(ma_format_f32, 0, 0);
    const ma_result result = ma_decoder_init_file(
        config_.filePath.c_str(),
        &decoderConfig,
        decoder_);
    if (result != MA_SUCCESS) {
        delete decoder_;
        decoder_ = nullptr;
        return false;
    }

    ma_uint32 decodedChannels = 0;
    ma_uint32 decodedSampleRate = 0;
    ma_decoder_get_data_format(
        decoder_, nullptr, &decodedChannels, &decodedSampleRate, nullptr, 0);

    ma_uint64 expectedFrames = 0;
    if (decodedChannels == 0 || decodedSampleRate == 0 ||
        ma_decoder_get_length_in_pcm_frames(decoder_, &expectedFrames) != MA_SUCCESS ||
        expectedFrames > std::numeric_limits<std::size_t>::max() / decodedChannels) {
        return false;
    }

    const std::size_t expectedSamples =
        static_cast<std::size_t>(expectedFrames) * decodedChannels;
    std::vector<float> decodedPcm(expectedSamples);

    ma_uint64 framesRead = 0;
    while (framesRead < expectedFrames) {
        ma_uint64 framesReadThisPass = 0;
        const ma_result readResult = ma_decoder_read_pcm_frames(
            decoder_,
            decodedPcm.data() + static_cast<std::size_t>(framesRead) * decodedChannels,
            expectedFrames - framesRead,
            &framesReadThisPass);

        framesRead += framesReadThisPass;
        if (readResult != MA_SUCCESS && readResult != MA_AT_END) {
            return false;
        }

        if (framesReadThisPass == 0 || readResult == MA_AT_END) {
            break;
        }
    }

    decodedPcm.resize(static_cast<std::size_t>(framesRead) * decodedChannels);

    std::lock_guard lock{mutex_};
    latestSamples_ = std::move(decodedPcm);
    sampleRate_ = decodedSampleRate;
    channelCount_ = decodedChannels;
    frameCount_ = framesRead;
    initialized_ = true;
    deviceInitialized_ = true;
    return true;
}

void AudioInput::ResetState() noexcept {
    std::lock_guard lock{mutex_};
    latestSamples_.clear();
    sampleRate_ = 0;
    channelCount_ = 0;
    frameCount_ = 0;
}

void AudioInput::CaptureCallback(
    ma_device* device,
    void* output,
    const void* input,
    ma_uint32 frameCount) {
    static_cast<void>(output);

    auto* self = static_cast<AudioInput*>(device->pUserData);
    if (self == nullptr || input == nullptr) {
        return;
    }

    const auto* samples = static_cast<const float*>(input);
    const auto sampleCount = static_cast<std::size_t>(frameCount) * self->config_.channelCount;

    std::lock_guard lock{self->mutex_};
    self->latestSamples_.assign(samples, samples + sampleCount);
    self->sampleRate_ = self->config_.sampleRate;
    self->channelCount_ = self->config_.channelCount;
    self->frameCount_ = static_cast<std::uint64_t>(frameCount);
}

} // namespace papagedon::audio
