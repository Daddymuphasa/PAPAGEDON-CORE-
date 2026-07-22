#include "AudioInput.hpp"

#include <limits>
#include <utility>

#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>

namespace aei {

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
    if (decodedChannels == 0 || decodedSampleRate == 0 ||
        ma_decoder_get_length_in_pcm_frames(&decoder, &expectedFrames) != MA_SUCCESS ||
        expectedFrames > std::numeric_limits<std::size_t>::max() / decodedChannels) {
        return false;
    }

    std::vector<float> decodedPcm;
    try {
        decodedPcm.resize(static_cast<std::size_t>(expectedFrames) * decodedChannels);
    } catch (...) {
        return false;
    }

    ma_uint64 framesRead = 0;
    while (framesRead < expectedFrames) {
        ma_uint64 framesReadThisPass = 0;
        const ma_result result = ma_decoder_read_pcm_frames(
            &decoder,
            decodedPcm.data() + static_cast<std::size_t>(framesRead) * decodedChannels,
            expectedFrames - framesRead,
            &framesReadThisPass);

        framesRead += framesReadThisPass;
        if (result != MA_SUCCESS && result != MA_AT_END) {
            return false;
        }
        if (framesReadThisPass == 0 || result == MA_AT_END) {
            break;
        }
    }

    decodedPcm.resize(static_cast<std::size_t>(framesRead) * decodedChannels);
    pcmData_ = std::move(decodedPcm);
    sampleRate_ = decodedSampleRate;
    channels_ = decodedChannels;
    frameCount_ = framesRead;
    return true;
}

void AudioInput::Close() noexcept {
    std::vector<float>{}.swap(pcmData_);
    sampleRate_ = 0;
    channels_ = 0;
    frameCount_ = 0;
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

} // namespace aei
