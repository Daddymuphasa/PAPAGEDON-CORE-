#include "AutoDirector.h"

#include <array>
#include <cmath>
#include <cstddef>

namespace papagedon {

namespace {

// ── Preset character profiles ──────────────────────────────────────────────
// Each preset has an intrinsic affinity for certain frequency bands and moods.
// These are hand-tuned to the visual character of each form:
//   bassBias  >0 means the preset looks best with bass-heavy music
//   trebleBias >0 means the preset looks best with treble-heavy music
//   warmth    [0,1] how warm/euphoric vs dark/cold the preset reads
struct PresetProfile {
    PresetId id;
    int      tier;        // 0=calm, 1=groove, 2=peak
    float    bassBias;
    float    trebleBias;
    float    warmth;      // preferred mood match
};

constexpr std::array<PresetProfile, 12> kProfiles{{
    // Calm tier — ambient, flowing, introspective
    {PresetId::Aurora,    0,  0.1f, 0.0f, 0.30f},  // curtains — warm, slow
    {PresetId::Nebula,    0,  0.3f, 0.0f, 0.25f},  // clouds — deep, spacey
    {PresetId::Liquid,    0,  0.4f, 0.0f, 0.20f},  // fluid — sub bass, dark

    // Groove tier — structured, rhythmic, building
    {PresetId::Matrix,    1,  0.0f, 0.4f, 0.40f},  // digital rain — crisp, mid-heavy
    {PresetId::Mandala,   1,  0.2f, 0.2f, 0.50f},  // kaleidoscope — balanced
    {PresetId::Lattice,   1,  0.1f, 0.3f, 0.45f},  // neon grid — treble-reactive
    {PresetId::Spectrum,  1,  0.3f, 0.1f, 0.55f},  // circular bars — bass-forward
    {PresetId::Vortex,    1,  0.3f, 0.2f, 0.60f},  // spiral — hypnotic, bass-driven

    // Peak tier — intense, explosive, maximal
    {PresetId::Shockwave, 2,  0.5f, 0.1f, 0.80f},  // bass shockwaves — pure sub
    {PresetId::Lasers,    2,  0.1f, 0.5f, 0.70f},  // sweeping lasers — treble-reactive
    {PresetId::Pulse,     2,  0.3f, 0.3f, 0.75f},  // radial pulse — balanced energy
    {PresetId::Tunnel,    2,  0.4f, 0.2f, 0.85f},  // warp tunnel — bass-driven rush
}};

// ── Timing (seconds) ────────────────────────────────────────────────────
constexpr float kMinDwell   = 6.0F;
constexpr float kMaxDwell   = 14.0F;
constexpr float kDropCutMin = 3.0F;

} // namespace

std::uint32_t AutoDirector::NextRandom() noexcept {
    rng_ ^= rng_ << 13;
    rng_ ^= rng_ >> 17;
    rng_ ^= rng_ << 5;
    return rng_;
}

int AutoDirector::TierFor(const float env, const ExperienceState state) const noexcept {
    if (mode_ == ShowMode::Trance) return 0;
    if (mode_ == ShowMode::Badman) return env >= 0.55F ? 2 : 1;
    if (state == ExperienceState::Silence || env < 0.12F) return 0;
    if (state == ExperienceState::Drop    || env >= 0.55F) return 2;
    return 1;
}

PresetId AutoDirector::PickFromTier(const int tier, const PresetId avoid) noexcept {
    // Score every preset and pick the best match for the current audio character.
    float bestScore  = -1e9F;
    PresetId bestPick = avoid;

    for (const auto& p : kProfiles) {
        if (p.tier != tier) continue;

        float score = 0.0F;

        // Frequency profile match: bass-heavy music favours bass-biased presets.
        const float freqBalance = smoothBass_ - smoothTreble_;
        score += freqBalance * p.bassBias * 4.0F;
        score += (smoothTreble_ - smoothBass_) * p.trebleBias * 4.0F;

        // Mood match: how close is the preset's warmth to the current mood.
        const float moodDist = std::abs(smoothMood_ - p.warmth);
        score -= moodDist * 2.0F;

        // Variety: penalise the currently playing preset.
        if (p.id == avoid) score -= 6.0F;

        // Small random jitter so ties don't always resolve identically.
        score += static_cast<float>(NextRandom() % 100) * 0.005F;

        if (score > bestScore) {
            bestScore = score;
            bestPick  = p.id;
        }
    }
    return bestPick;
}

PresetId AutoDirector::Update(const ExperienceGraphOutput& e, const float dt) noexcept {
    // Smooth the frequency bands and mood over several seconds so selections
    // track the section character, not individual transients.
    const float sa = 1.0F - std::exp(-dt * 0.6F);
    smoothBass_   += (e.bass   - smoothBass_)   * sa;
    smoothTreble_ += (e.treble - smoothTreble_) * sa;
    smoothMood_   += (e.mood   - smoothMood_)   * sa;

    if (!initialized_) {
        currentTier_ = TierFor(e.energy, e.state);
        current_     = PickFromTier(currentTier_, PresetId::Aurora);
        energyEnv_   = e.energy;
        initialized_ = true;
        return current_;
    }

    const float alpha = 1.0F - std::exp(-dt * 1.5F);
    energyEnv_ += (e.energy - energyEnv_) * alpha;
    dwell_     += dt;

    const bool dropEdge = (e.event == ExperienceEvent::Drop &&
                           lastEvent_ != ExperienceEvent::Drop);
    lastEvent_ = e.event;

    const int tier = TierFor(energyEnv_, e.state);

    bool switchNow = false;
    const bool gentle = mode_ == ShowMode::Trance;
    if (!gentle && dropEdge && dwell_ >= kDropCutMin) {
        switchNow = true;
    } else if (dwell_ >= kMinDwell && tier != currentTier_) {
        switchNow = true;
    } else if (dwell_ >= (gentle ? 24.0F : kMaxDwell)) {
        switchNow = true;
    }

    if (switchNow) {
        currentTier_ = tier;
        current_     = PickFromTier(tier, current_);
        dwell_       = 0.0F;
    }
    return current_;
}

void AutoDirector::Reset() noexcept {
    current_      = PresetId::Aurora;
    currentTier_  = 0;
    energyEnv_    = 0.0F;
    dwell_        = 0.0F;
    lastEvent_    = ExperienceEvent::Silence;
    initialized_  = false;
    smoothBass_   = 0.0F;
    smoothTreble_ = 0.0F;
    smoothMood_   = 0.0F;
}

void AutoDirector::SetMode(const ShowMode mode) noexcept {
    if (mode_ == mode) return;
    mode_ = mode;
    Reset();
}

} // namespace papagedon
