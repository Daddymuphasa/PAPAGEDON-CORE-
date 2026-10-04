#include "AudioCapture.h"

// miniaudio's implementation lives in AudioInput.cpp; here we only use its API.
#include <miniaudio.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>

namespace papagedon::audio {

class AudioCapture::Impl final {
public:
    ma_context context{};
    ma_device  device{};
    bool contextReady = false;
    bool deviceReady  = false;
    bool started      = false;

    std::uint32_t sampleRate = 44100;
    std::uint32_t channels   = 2;
    std::string   deviceName = "None";

    // Ring buffer (~2 s), interleaved float.
    std::uint64_t ringFrames = 88200;
    std::unique_ptr<std::atomic<float>[]> ring;
    std::atomic<std::uint64_t> writeFrame{0};
    std::atomic<std::int64_t> lastCallbackNs{0};
    std::atomic<float> jitterMs{0.0f};
    std::atomic<unsigned> observations{0};
    float periodMs = 0.0f;
    std::int64_t previousCallbackNs = 0;

    void Write(const float* src, ma_uint32 frames) noexcept {
        const std::uint64_t w = writeFrame.load(std::memory_order_relaxed);
        const std::uint32_t ch = channels;
        for (ma_uint32 i = 0; i < frames; ++i) {
            const std::uint64_t idx = ((w + i) % ringFrames) * ch;
            for (std::uint32_t c = 0; c < ch; ++c) {
                ring[static_cast<std::size_t>(idx + c)].store(src[i * ch + c], std::memory_order_relaxed);
            }
        }
        writeFrame.store(w + frames, std::memory_order_release);
        const auto now = std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
        if (previousCallbackNs > 0) {
            const float intervalMs = static_cast<float>(now - previousCallbackNs) / 1.0e6f;
            const float expectedMs = frames * 1000.0f / sampleRate;
            const float jitter = std::abs(intervalMs - expectedMs);
            const float old = jitterMs.load(std::memory_order_relaxed);
            jitterMs.store(old + (jitter - old) * 0.04f, std::memory_order_relaxed);
        }
        previousCallbackNs = now;
        lastCallbackNs.store(now, std::memory_order_release);
        observations.fetch_add(1, std::memory_order_relaxed);
    }

    // miniaudio data callback (runs on the audio thread).
    static void Callback(ma_device* device, void* /*output*/, const void* input,
                         ma_uint32 frameCount) {
        auto* impl = static_cast<Impl*>(device->pUserData);
        if (impl != nullptr && input != nullptr) {
            impl->Write(static_cast<const float*>(input), frameCount);
        }
    }
};

AudioCapture::AudioCapture() : impl_{std::make_unique<Impl>()} {}

AudioCapture::~AudioCapture() {
    Shutdown();
}

bool AudioCapture::Initialize(const int deviceIndex, const bool loopback) {
    if (ma_context_init(nullptr, 0, nullptr, &impl_->context) != MA_SUCCESS) {
        return false;
    }
    impl_->contextReady = true;

    ma_device_info* playbackInfos = nullptr;
    ma_device_info* captureInfos  = nullptr;
    ma_uint32 playbackCount = 0;
    ma_uint32 captureCount  = 0;
    ma_context_get_devices(&impl_->context, &playbackInfos, &playbackCount,
                           &captureInfos, &captureCount);

    ma_device_config config;
    if (loopback) {
        config = ma_device_config_init(ma_device_type_loopback);
        if (deviceIndex >= 0 && deviceIndex < static_cast<int>(playbackCount)) {
            config.playback.pDeviceID = &playbackInfos[deviceIndex].id;
            impl_->deviceName = std::string("Loopback: ") + playbackInfos[deviceIndex].name;
        } else {
            impl_->deviceName = "Loopback: default output";
        }
    } else {
        config = ma_device_config_init(ma_device_type_capture);
        if (deviceIndex >= 0 && deviceIndex < static_cast<int>(captureCount)) {
            config.capture.pDeviceID = &captureInfos[deviceIndex].id;
            impl_->deviceName = captureInfos[deviceIndex].name;
        } else {
            impl_->deviceName = "Default input";
        }
    }

    config.capture.format   = ma_format_f32;
    config.capture.channels = impl_->channels;   // request stereo (miniaudio converts)
    config.sampleRate       = impl_->sampleRate; // request 44.1 kHz
    config.dataCallback     = &Impl::Callback;
    config.pUserData        = impl_.get();

    // ── Low-latency buffering ───────────────────────────────────────────────────
    // This is a live VJ tool: the booth feed must reach the visuals fast.  The
    // callback only copies into a lock-free ring buffer, so a short buffer is
    // safe (no heavy work to starve the audio thread).  Request the smallest
    // practical period; WASAPI shared mode clamps up to the device minimum, so
    // this lowers latency without risking dropouts.  PAPAGEDON_CAPTURE_PERIOD
    // overrides the period size (in frames) for tuning at soundcheck.
    config.performanceProfile = ma_performance_profile_low_latency;
    ma_uint32 periodFrames = 128; // Shared-mode drivers clamp to their supported minimum.
    if (const char* const p = std::getenv("PAPAGEDON_CAPTURE_PERIOD")) {
        const int v = std::atoi(p);
        if (v >= 32 && v <= 4096) {
            periodFrames = static_cast<ma_uint32>(v);
        }
    }
    config.periodSizeInFrames = periodFrames;
    config.periods            = 2; // minimal double-buffer

    if (ma_device_init(&impl_->context, &config, &impl_->device) != MA_SUCCESS) {
        ma_context_uninit(&impl_->context);
        impl_->contextReady = false;
        return false;
    }
    impl_->deviceReady = true;

    // Honour whatever the device actually settled on.
    impl_->channels   = impl_->device.capture.channels;
    impl_->sampleRate = impl_->device.sampleRate != 0 ? impl_->device.sampleRate : 44100;
    impl_->ringFrames = static_cast<std::uint64_t>(impl_->sampleRate) * 2;
    impl_->ring = std::make_unique<std::atomic<float>[]>(static_cast<std::size_t>(impl_->ringFrames) * impl_->channels);
    impl_->writeFrame.store(0, std::memory_order_release);
    impl_->lastCallbackNs.store(0);
    impl_->jitterMs.store(0.0f);
    impl_->observations.store(0);
    impl_->previousCallbackNs = 0;
    const auto nativeRate = std::max(impl_->device.capture.internalSampleRate, 1u);
    impl_->periodMs = 1000.0f * impl_->device.capture.internalPeriodSizeInFrames /
        static_cast<float>(nativeRate);
    return true;
}

bool AudioCapture::Start() {
    if (!impl_->deviceReady) {
        return false;
    }
    if (ma_device_start(&impl_->device) != MA_SUCCESS) {
        return false;
    }
    impl_->started = true;
    return true;
}

void AudioCapture::Stop() {
    if (impl_->deviceReady && impl_->started) {
        ma_device_stop(&impl_->device);
        impl_->started = false;
    }
}

void AudioCapture::Shutdown() noexcept {
    if (impl_ == nullptr) {
        return;
    }
    if (impl_->deviceReady) {
        ma_device_uninit(&impl_->device);
        impl_->deviceReady = false;
    }
    if (impl_->contextReady) {
        ma_context_uninit(&impl_->context);
        impl_->contextReady = false;
    }
}

bool AudioCapture::IsOpen() const noexcept { return impl_->deviceReady; }
std::uint32_t AudioCapture::SampleRate() const noexcept { return impl_->sampleRate; }
std::uint32_t AudioCapture::Channels() const noexcept { return impl_->channels; }
const std::string& AudioCapture::DeviceName() const noexcept { return impl_->deviceName; }

std::size_t AudioCapture::ReadLatest(std::vector<float>& out, const std::size_t frames) const {
    const std::uint32_t ch = impl_->channels;
    out.assign(frames * ch, 0.0F);
    const std::uint64_t w = impl_->writeFrame.load(std::memory_order_acquire);
    const std::size_t avail =
        static_cast<std::size_t>(std::min<std::uint64_t>(w, frames));
    const std::uint64_t start = w - avail;
    for (std::size_t i = 0; i < avail; ++i) {
        const std::uint64_t idx = ((start + i) % impl_->ringFrames) * ch;
        for (std::uint32_t c = 0; c < ch; ++c) {
            out[(frames - avail + i) * ch + c] = impl_->ring[static_cast<std::size_t>(idx + c)].load(std::memory_order_relaxed);
        }
    }
    return avail;
}

double AudioCapture::CapturedSeconds() const noexcept {
    const std::uint64_t w = impl_->writeFrame.load(std::memory_order_acquire);
    return impl_->sampleRate > 0 ? static_cast<double>(w) / impl_->sampleRate : 0.0;
}

CaptureTiming AudioCapture::Timing() const noexcept {
    CaptureTiming result;
    result.observations = impl_->observations.load(std::memory_order_relaxed);
    result.periodMs = impl_->periodMs;
    result.jitterMs = impl_->jitterMs.load(std::memory_order_relaxed);
    const auto last = impl_->lastCallbackNs.load(std::memory_order_acquire);
    const auto now = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    result.ageMs = last > 0 ? static_cast<float>(now - last) / 1.0e6f : 0.0f;
    return result;
}

void AudioCapture::ListDevices() {
    ma_context context;
    if (ma_context_init(nullptr, 0, nullptr, &context) != MA_SUCCESS) {
        std::printf("AudioCapture: could not initialise audio context.\n");
        return;
    }
    ma_device_info* playbackInfos = nullptr;
    ma_device_info* captureInfos  = nullptr;
    ma_uint32 playbackCount = 0;
    ma_uint32 captureCount  = 0;
    if (ma_context_get_devices(&context, &playbackInfos, &playbackCount,
                               &captureInfos, &captureCount) == MA_SUCCESS) {
        std::printf("\nInput / capture devices (use with PAPAGEDON_CAPTURE_DEVICE=<n>):\n");
        for (ma_uint32 i = 0; i < captureCount; ++i) {
            std::printf("  [%u] %s%s\n", i, captureInfos[i].name,
                        captureInfos[i].isDefault ? "  (default)" : "");
        }
        std::printf("\nOutput devices (loopback with PAPAGEDON_AUDIO=loopback PAPAGEDON_CAPTURE_DEVICE=<n>):\n");
        for (ma_uint32 i = 0; i < playbackCount; ++i) {
            std::printf("  [%u] %s%s\n", i, playbackInfos[i].name,
                        playbackInfos[i].isDefault ? "  (default)" : "");
        }
        std::printf("\n");
    }
    ma_context_uninit(&context);
}

std::vector<AudioCapture::DeviceInfo> AudioCapture::EnumerateDevices() {
    std::vector<DeviceInfo> result;
    ma_context context;
    if (ma_context_init(nullptr, 0, nullptr, &context) != MA_SUCCESS) {
        return result;
    }
    ma_device_info* playbackInfos = nullptr;
    ma_device_info* captureInfos  = nullptr;
    ma_uint32 playbackCount = 0;
    ma_uint32 captureCount  = 0;
    if (ma_context_get_devices(&context, &playbackInfos, &playbackCount,
                               &captureInfos, &captureCount) == MA_SUCCESS) {
        for (ma_uint32 i = 0; i < playbackCount; ++i) {
            result.push_back({playbackInfos[i].name,
                              static_cast<int>(i),
                              playbackInfos[i].isDefault != 0,
                              false});
        }
        for (ma_uint32 i = 0; i < captureCount; ++i) {
            result.push_back({captureInfos[i].name,
                              static_cast<int>(i),
                              captureInfos[i].isDefault != 0,
                              true});
        }
    }
    ma_context_uninit(&context);
    return result;
}

} // namespace papagedon::audio
