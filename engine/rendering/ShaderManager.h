#pragma once

#include "ShaderUniforms.h"

namespace papagedon {

// ──────────────────────────────────────────────────────────────────────────────
// ShaderManager
//
// Compiles a vertex + fragment GLSL pair into a linked program.
// Caches all uniform locations at compile time so per-frame SetUniforms()
// is branch-free and performs only glUniform* calls.
//
// Lifetime rules:
//   - Must be used only after a valid OpenGL context exists.
//   - Compile() must succeed before Bind() or SetUniforms().
//   - Shutdown() must be called before the OpenGL context is destroyed.
// ──────────────────────────────────────────────────────────────────────────────
class ShaderManager final {
public:
    ShaderManager() = default;
    ~ShaderManager();

    ShaderManager(const ShaderManager&) = delete;
    ShaderManager& operator=(const ShaderManager&) = delete;

    /// Compile and link vertex + fragment source strings.
    /// Logs all GLSL errors to stderr.
    /// Returns false on any compile or link failure; the manager remains invalid.
    [[nodiscard]] bool Compile(
        const char* vertexSource,
        const char* fragmentSource) noexcept;

    /// Activate this program for subsequent draw calls.
    void Bind() const noexcept;

    /// Upload all per-frame uniforms.
    /// width/height are the current framebuffer dimensions (pixels).
    /// time is the wall-clock seconds since the renderer started.
    void SetUniforms(
        const ShaderUniforms& uniforms,
        float time,
        int width,
        int height) const noexcept;

    void Shutdown() noexcept;

    [[nodiscard]] bool IsValid() const noexcept { return program_ != 0; }

    /// Returns the embedded default vertex GLSL (fullscreen triangle).
    [[nodiscard]] static const char* DefaultVertexSource() noexcept;

    /// Returns the embedded default reactive fragment GLSL.
    [[nodiscard]] static const char* DefaultFragmentSource() noexcept;

private:
    unsigned int program_ = 0;

    // Cached uniform locations — resolved once in Compile().
    int locTime_       = -1;
    int locResolution_ = -1;
    int locEnergy_     = -1;
    int locIntensity_  = -1;
    int locBass_       = -1;
    int locMid_        = -1;
    int locTreble_     = -1;
    int locBeat_       = -1;
    int locMood_       = -1;
    int locPrimaryColour_   = -1;
    int locSecondaryColour_ = -1;
    int locAccentColour_    = -1;
    int locBackground_      = -1;
    int locGlow_            = -1;
    int locBloom_           = -1;
    int locMotion_          = -1;
    int locNoise_           = -1;
    int locDistortion_      = -1;
    int locSaturationBase_  = -1;
    int locSaturationScale_ = -1;
    int locPattern_         = -1;
    int locPrevPattern_     = -1;
    int locPatternBlend_    = -1;
    int locDetail_          = -1;
    int locMasterBrightness_ = -1;
    int locMasterGlow_       = -1;
    int locMasterExposure_   = -1;
    int locFeedbackDecay_    = -1;
    int locFeedbackZoom_     = -1;
    int locFeedbackRotation_ = -1;
    int locFeedbackWarp_     = -1;
    int locFeedbackBeatWarp_ = -1;
    int locWaveformMode_     = -1;
    int locWaveformOpacity_  = -1;
    int locWaveformThickness_ = -1;
    int locWaveformRadius_   = -1;
    int locWaveformBassResponse_ = -1;
    int locWaveformTrebleResponse_ = -1;
};

} // namespace papagedon
