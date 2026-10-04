#include <AutoDirector.h>
#include <ThemeManager.h>
#include <InputLatency.h>
#include <DoubleTap.h>
#include <AudioAnalyzer.h>

#include <array>
#include <cstdlib>
#include <iostream>

using namespace papagedon;

void Require(bool value, const char* message) {
    if (!value) { std::cerr << message << '\n'; std::exit(1); }
}

bool Gentle(PresetId id) {
    return id == PresetId::Aurora || id == PresetId::Nebula || id == PresetId::Liquid;
}

int main() {
    DoubleTap tap;
    Require(!tap.Press(1.0), "first tap was incorrectly treated as double");
    Require(tap.Press(1.35), "double tap was not detected");
    Require(!tap.Press(2.0), "double tap latch did not reset");
    Require(!tap.Press(2.6), "slow taps were treated as double");
    audio::InputLatency latency;
    latency.Update({.periodMs=4.0f, .jitterMs=0.4f, .ageMs=2.0f, .observations=128}, 48000.0f, 0.016f);
    Require(latency.EstimatedMs() > 15.0f && latency.EstimatedMs() < 18.0f,
            "input latency estimate excluded an analysis window");
    Require(latency.AttackRate() >= 100.0f, "latency calibrator slowed visual attack");
    latency.Update({.periodMs=4.0f, .jitterMs=0.4f, .ageMs=120.0f, .observations=128}, 48000.0f, 0.016f);
    Require(latency.Stalled(), "stalled capture was not detected");
    audio::AudioAnalyzer analyzer;
    std::array<float, 2048> samples{};
    for (std::size_t i = 0; i < samples.size(); ++i)
        samples[i] = i > 1800 ? 0.8f : 0.0f;
    audio::AudioFrame frame{samples, 48000, 2, 1.0};
    const auto first = analyzer.Update(frame);
    const auto held = analyzer.Update(frame);
    Require(!held.beat, "re-reading one capture window repeated a beat");
    Require(first.waveform.back() > 0.7f, "waveform history was Hann-windowed instead of raw PCM");
    Require(!SceneAllowed(ShowMode::Trance, ShowMode::Badman), "TRANCE admitted a party scene");
    Require(!SceneAllowed(ShowMode::Badman, ShowMode::Trance), "Badman admitted a TRANCE scene");
    Require(SceneAllowed(ShowMode::Open, ShowMode::Badman), "Open mode lost library access");
    visual::ThemeManager themes;
    Require(themes.SetTheme("trance"), "TRANCE fallback missing");
    Require(themes.SetTheme("badman"), "Badman fallback missing");
    Require(themes.LoadTheme("engine/rendering/shaders/TranceExperiencePack/trance-theme.json"), "TRANCE JSON failed");
    Require(themes.CurrentTheme().motion < 0.65f, "TRANCE motion settings were ignored");

    AutoDirector director;
    director.SetMode(ShowMode::Trance);
    std::array<bool, kPresetCount> seen{};
    ExperienceGraphOutput audio;
    for (int frame = 0; frame < 7200; ++frame) {
        audio.energy = frame % 900 < 450 ? 0.95f : 0.03f;
        audio.state = audio.energy > 0.5f ? ExperienceState::Drop : ExperienceState::Silence;
        audio.event = audio.energy > 0.5f ? ExperienceEvent::Drop : ExperienceEvent::Silence;
        auto id = director.Update(audio, 0.05f);
        Require(Gentle(id), "TRANCE escaped its gentle presets during a drop");
        seen[static_cast<std::size_t>(id)] = true;
    }
    int variety = 0;
    for (bool value : seen) variety += value;
    Require(variety == 3, "TRANCE did not explore all its forms over six minutes");
    director.SetMode(ShowMode::Badman);
    for (int frame = 0; frame < 2400; ++frame) {
        audio.energy = frame % 600 < 300 ? 0.0f : 0.9f;
        Require(!Gentle(director.Update(audio, 0.05f)), "Badman reverted to a calm preset");
    }
    director.SetMode(ShowMode::Trance);
    Require(Gentle(director.Update(audio, 0.016f)), "Switching back to TRANCE was delayed");
    std::cout << "Section boundaries, six-minute variety, theme loading, and switching passed.\n";
}
