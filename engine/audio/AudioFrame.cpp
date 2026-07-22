#include "AudioFrame.h"

namespace papagedon::audio {

bool AudioFrame::IsValid() const noexcept {
    return sampleRate > 0 && channelCount > 0 &&
           samples.size() % channelCount == 0;
}

std::size_t AudioFrame::FrameCount() const noexcept {
    return IsValid() ? samples.size() / channelCount : 0;
}

} // namespace papagedon::audio
