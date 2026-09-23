#include "ExperienceSignals.h"

#include <cmath>

namespace papagedon::audio {

bool ExperienceSignals::IsFinite() const noexcept {
    bool finite = std::isfinite(energy) && std::isfinite(intensity) &&
                  std::isfinite(bass) && std::isfinite(mid) && std::isfinite(treble) &&
                  std::isfinite(tension) && std::isfinite(bpm) &&
                  std::isfinite(confidence);
    for (float v : spectrum) {
        finite = finite && std::isfinite(v);
    }
    for (float v : waveform) {
        finite = finite && std::isfinite(v);
    }
    return finite;
}

} // namespace papagedon::audio
