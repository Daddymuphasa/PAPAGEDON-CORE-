#include "ExperienceSignals.h"

#include <cmath>

namespace papagedon::audio {

bool ExperienceSignals::IsFinite() const noexcept {
    return std::isfinite(energy) && std::isfinite(intensity) &&
           std::isfinite(bass) && std::isfinite(mid) && std::isfinite(treble) &&
           std::isfinite(tension) && std::isfinite(bpm) &&
           std::isfinite(confidence);
}

} // namespace papagedon::audio
