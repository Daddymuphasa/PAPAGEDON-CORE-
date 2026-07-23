#include "ExperienceSignals.h"

#include <cmath>

namespace papagedon::audio {

bool ExperienceSignals::IsFinite() const noexcept {
    if (!(std::isfinite(energy) && std::isfinite(intensity) &&
          std::isfinite(tension) && std::isfinite(bpm) &&
          std::isfinite(beatConfidence) && std::isfinite(confidence) &&
          std::isfinite(bassEnergy) && std::isfinite(midEnergy) &&
          std::isfinite(trebleEnergy) && std::isfinite(bassLevel) &&
          std::isfinite(midLevel) && std::isfinite(trebleLevel) &&
          std::isfinite(loudness) && std::isfinite(dynamicRange))) {
        return false;
    }

    for (const float magnitude : spectrum) {
        if (!std::isfinite(magnitude)) {
            return false;
        }
    }

    return true;
}

} // namespace papagedon::audio
