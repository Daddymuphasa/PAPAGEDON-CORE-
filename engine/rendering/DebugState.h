#pragma once

namespace papagedon {

/// Real-time engine state for the debug overlay
struct DebugState final {
    float fps = 0.0F;
    float bpm = 0.0F;
    float energy = 0.0F;
    float intensity = 0.0F;
    float bass = 0.0F;
    float mid = 0.0F;
    float treble = 0.0F;
    bool  beat = false;
    const char* currentExperience = "";
    const char* currentScene = "";
    const char* currentPreset = "";
    const char* currentTheme = "";
    const char* currentShader = "";
    const char* currentAudioFile = "";
    const char* rendererBackend = "";
    int   windowWidth = 0;
    int   windowHeight = 0;
    bool  autoMode = false;
    float transitionProgress = 0.0F;
};

} // namespace papagedon
