#include "AudioPlayer.h"
#include "AudioInput.h"

#include <miniaudio.h>
#include <atomic>
#include <algorithm>
#include <cstring>

namespace papagedon::audio {

class AudioPlayer::Implementation {
public:
    ma_device device;
    bool deviceInitialized = false;

    const AudioInput* currentInput = nullptr;
    std::atomic<std::uint64_t> currentFrame{0};
    std::atomic<bool> isPlaying{false};

    static void DataCallback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
        auto* impl = static_cast<Implementation*>(pDevice->pUserData);
        if (!impl || !impl->isPlaying.load(std::memory_order_acquire) || !impl->currentInput) {
            std::memset(pOutput, 0, frameCount * pDevice->playback.channels * sizeof(float));
            return;
        }

        const std::uint64_t current = impl->currentFrame.load(std::memory_order_relaxed);
        const std::uint64_t totalFrames = impl->currentInput->FrameCount();
        const std::uint32_t channels = impl->currentInput->Channels();

        if (current >= totalFrames) {
            std::memset(pOutput, 0, frameCount * channels * sizeof(float));
            impl->isPlaying.store(false, std::memory_order_release);
            return;
        }

        const std::uint64_t framesRemaining = totalFrames - current;
        const std::uint32_t framesToRead = static_cast<std::uint32_t>(std::min<std::uint64_t>(frameCount, framesRemaining));

        const std::span<const float> samples = impl->currentInput->GetSamples();
        const float* src = samples.data() + current * channels;
        float* dst = static_cast<float*>(pOutput);

        std::memcpy(dst, src, framesToRead * channels * sizeof(float));

        if (framesToRead < frameCount) {
            std::memset(dst + framesToRead * channels, 0, (frameCount - framesToRead) * channels * sizeof(float));
            impl->isPlaying.store(false, std::memory_order_release);
        }

        impl->currentFrame.fetch_add(framesToRead, std::memory_order_relaxed);
        (void)pInput;
    }
};

AudioPlayer::AudioPlayer() : impl_(new Implementation{}) {}

AudioPlayer::~AudioPlayer() {
    Shutdown();
    delete impl_;
}

bool AudioPlayer::Initialize() {
    if (impl_->deviceInitialized) return true;

    ma_device_config deviceConfig = ma_device_config_init(ma_device_type_playback);
    deviceConfig.playback.format   = ma_format_f32;
    // We expect stereo at 48kHz for now, or we can use 0 for native and resample.
    // However AudioInput decodes at a specific rate. 
    // We will initialize the device when Load() is called to match the input format.
    // Wait, let's just initialize it to standard format and we'll reinit if needed, 
    // or just require the AudioInput format.
    // To be safe, we just leave it default and miniaudio will request format from OS, 
    // but we need to match what we output. 
    // Actually, miniaudio can resample if we use an `ma_decoder`. Since we decode everything to float, 
    // we should configure the device to the input's sample rate when playing.
    return true; // We defer actual miniaudio init until Load() so we have the format.
}

void AudioPlayer::Load(const AudioInput* input) {
    if (impl_->deviceInitialized) {
        ma_device_uninit(&impl_->device);
        impl_->deviceInitialized = false;
    }

    impl_->currentInput = input;
    impl_->currentFrame.store(0, std::memory_order_relaxed);
    impl_->isPlaying.store(false, std::memory_order_release);

    if (!input) return;

    ma_device_config deviceConfig = ma_device_config_init(ma_device_type_playback);
    deviceConfig.playback.format   = ma_format_f32;
    deviceConfig.playback.channels = input->Channels();
    deviceConfig.sampleRate        = input->SampleRate();
    deviceConfig.dataCallback      = Implementation::DataCallback;
    deviceConfig.pUserData         = impl_;

    if (ma_device_init(nullptr, &deviceConfig, &impl_->device) == MA_SUCCESS) {
        impl_->deviceInitialized = true;
    }
}

void AudioPlayer::Play() {
    if (impl_->deviceInitialized && impl_->currentInput) {
        impl_->isPlaying.store(true, std::memory_order_release);
        ma_device_start(&impl_->device);
    }
}

void AudioPlayer::Pause() {
    impl_->isPlaying.store(false, std::memory_order_release);
    if (impl_->deviceInitialized) {
        ma_device_stop(&impl_->device);
    }
}

void AudioPlayer::Stop() {
    Pause();
    Seek(0);
}

void AudioPlayer::Seek(std::uint64_t frameIndex) {
    if (impl_->currentInput) {
        const std::uint64_t total = impl_->currentInput->FrameCount();
        impl_->currentFrame.store(std::min(frameIndex, total), std::memory_order_relaxed);
    }
}

void AudioPlayer::Update() {
    // Polling or syncing logic if needed
}

std::uint64_t AudioPlayer::GetPlaybackPositionInFrames() const noexcept {
    return impl_->currentFrame.load(std::memory_order_acquire);
}

double AudioPlayer::GetCurrentPlaybackTime() const noexcept {
    if (!impl_->currentInput || impl_->currentInput->SampleRate() == 0) return 0.0;
    return static_cast<double>(GetPlaybackPositionInFrames()) / impl_->currentInput->SampleRate();
}

void AudioPlayer::Shutdown() noexcept {
    Stop();
    if (impl_ && impl_->deviceInitialized) {
        ma_device_uninit(&impl_->device);
        impl_->deviceInitialized = false;
    }
}

} // namespace papagedon::audio
