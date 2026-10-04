#include "AudioInput.h"

#include <limits>
#include <utility>

#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>

namespace papagedon::audio {

bool AudioInput::Load(const std::string& path) {
    Close();

    if (path.empty()) {
        return false;
    }

    ma_decoder decoder{};
    const ma_decoder_config config = ma_decoder_config_init(ma_format_f32, 0, 0);
    if (ma_decoder_init_file(path.c_str(), &config, &decoder) != MA_SUCCESS) {
        return false;
    }

    struct DecoderGuard final {
        ma_decoder& decoder;
        ~DecoderGuard() { ma_decoder_uninit(&decoder); }
    } guard{decoder};

    ma_uint32 decodedChannels = 0;
    ma_uint32 decodedSampleRate = 0;
    ma_decoder_get_data_format(
        &decoder, nullptr, &decodedChannels, &decodedSampleRate, nullptr, 0);

    ma_uint64 expectedFrames = 0;
    // We try to get the length to pre-allocate, but don't fail if we can't.
    ma_decoder_get_length_in_pcm_frames(&decoder, &expectedFrames);

    std::vector<float> decodedPcm;
    if (expectedFrames > 0 && expectedFrames < std::numeric_limits<std::size_t>::max() / decodedChannels) {
        try {
            decodedPcm.reserve(static_cast<std::size_t>(expectedFrames) * decodedChannels);
        } catch (...) {
            // Ignore reserve failure
        }
    }

    constexpr ma_uint64 CHUNK_FRAMES = 4096;
    std::vector<float> chunk(CHUNK_FRAMES * decodedChannels);

    ma_uint64 totalFramesRead = 0;
    while (true) {
        ma_uint64 framesReadThisPass = 0;
        const ma_result result = ma_decoder_read_pcm_frames(
            &decoder,
            chunk.data(),
            CHUNK_FRAMES,
            &framesReadThisPass);

        if (framesReadThisPass > 0) {
            decodedPcm.insert(decodedPcm.end(), chunk.data(), chunk.data() + (framesReadThisPass * decodedChannels));
            totalFramesRead += framesReadThisPass;
        }

        if (result != MA_SUCCESS || framesReadThisPass == 0) {
            break;
        }
    }

    if (totalFramesRead == 0) {
        return false;
    }

    decodedPcm.shrink_to_fit();
    pcmData_ = std::move(decodedPcm);
    sampleRate_ = decodedSampleRate;
    channels_ = decodedChannels;
    frameCount_ = totalFramesRead;
    return true;
}

void AudioInput::Close() noexcept {
    std::vector<float>{}.swap(pcmData_);
    sampleRate_ = 0;
    channels_ = 0;
    frameCount_ = 0;
}

std::span<const float> AudioInput::GetSamples() const noexcept {
    return pcmData_;
}

std::uint32_t AudioInput::SampleRate() const noexcept {
    return sampleRate_;
}

std::uint32_t AudioInput::Channels() const noexcept {
    return channels_;
}

std::uint64_t AudioInput::FrameCount() const noexcept {
    return frameCount_;
}

} // namespace papagedon::audio
