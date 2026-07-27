#include "OpenGLRenderer.h"
#include "../scene/SceneState.h"
#include "../ShaderUniforms.h"
#include "DebugOverlayRenderer.h"

#include <glad/glad.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <memory>
#include <sstream>
#include <string>

namespace papagedon {

// ──────────────────────────────────────────────────────────────────────────────
// Preset-colour helpers
//
// The active ExperiencePreset supplies every colour the shader uses.  These free
// functions adapt the experience layer's PresetColor into the renderer's Color3
// and ease one colour toward another so preset switches cross-fade on screen.
// No colours are hardcoded here — they all originate from the preset table.
// ──────────────────────────────────────────────────────────────────────────────
namespace {

Color3 ToColor3(const PresetColor& c) noexcept {
    return Color3{c.r, c.g, c.b};
}

Color3 LerpColor(const Color3& a, const Color3& b, const float t) noexcept {
    return Color3{
        a.r + (b.r - a.r) * t,
        a.g + (b.g - a.g) * t,
        a.b + (b.b - a.b) * t,
    };
}

// One-pole follower with asymmetric attack/release, frame-rate independent.
// A fast attack keeps visual onsets in sync with the audio (low latency); a
// slower release smooths the decay so nothing strobes between frames.
float Follow(const float current, const float target, const float dt,
             const float attackRate, const float releaseRate) noexcept {
    const float rate  = target > current ? attackRate : releaseRate;
    const float alpha = 1.0f - std::exp(-dt * rate);
    return current + (target - current) * alpha;
}

} // namespace

// ──────────────────────────────────────────────────────────────────────────────
// Implementation (pimpl)
// ──────────────────────────────────────────────────────────────────────────────
class OpenGLRenderer::Implementation final {
public:
    GLFWwindow* window              = nullptr;
    double      lastFpsUpdateTime   = 0.0;
    unsigned int renderedFrameCount = 0;
    double      currentFps          = 0.0;
    double      lastFrameTime       = 0.0;
    bool        showDebugOverlay    = false;
    bool        debugKeyWasPressed  = false;
    DebugOverlayRenderer debugOverlay;
    ShaderUniforms smoothedUniforms;

    // ── Pattern cross-fade state ───────────────────────────────────────────────
    // Tracks the signature form the shader is drawing.  When the preset's pattern
    // changes we snapshot the outgoing form and ease patternBlend 0 → 1, so the
    // visual form morphs across the switch rather than popping.
    int   activePatternMode   = 0;
    int   previousPatternMode = 0;
    float patternBlend        = 1.0f;
    bool  patternInitialized  = false;

    // ── Temporary preset controls (Stage 5.3) ─────────────────────────────────
    // F1..F6 select a preset.  Rising edges are latched in EndFrame (where events
    // are polled) and drained by the Runtime via ConsumePresetRequest.
    static constexpr int kPresetKeyCount = 6;
    bool presetKeyWasPressed[kPresetKeyCount] = {false, false, false, false, false, false};
    int  pendingPresetRequest = -1;
};

// ──────────────────────────────────────────────────────────────────────────────
// Lifetime
// ──────────────────────────────────────────────────────────────────────────────
OpenGLRenderer::OpenGLRenderer(const bool vsyncEnabled)
    : implementation_{std::make_unique<Implementation>()},
      vsyncEnabled_{vsyncEnabled} {}

OpenGLRenderer::~OpenGLRenderer() {
    Shutdown();
}

// ──────────────────────────────────────────────────────────────────────────────
// Initialize
// ──────────────────────────────────────────────────────────────────────────────
bool OpenGLRenderer::Initialize() {
    if (initialized_) {
        return true;
    }

    // ── Window & context ────────────────────────────────────────────────────
    if (glfwInit() != GLFW_TRUE) {
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);

    implementation_->window = glfwCreateWindow(
        1280, 720,
        "PAPAGEDON Core v0.0.1",
        nullptr, nullptr);

    if (implementation_->window == nullptr) {
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(implementation_->window);
    if (gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)) == 0) {
        glfwDestroyWindow(implementation_->window);
        implementation_->window = nullptr;
        glfwTerminate();
        return false;
    }

    glfwSwapInterval(vsyncEnabled_ ? 1 : 0);

    // ── Fullscreen VAO ──────────────────────────────────────────────────────
    // No vertex data is needed — the vertex shader generates positions from
    // gl_VertexID.  An empty VAO is still required by the OpenGL core profile.
    glGenVertexArrays(1, &fullscreenVAO_);

    // ── Shader ──────────────────────────────────────────────────────────────
    if (!shaderManager_.Compile(
            ShaderManager::DefaultVertexSource(),
            ShaderManager::DefaultFragmentSource())) {
        glDeleteVertexArrays(1, &fullscreenVAO_);
        fullscreenVAO_ = 0u;
        glfwDestroyWindow(implementation_->window);
        implementation_->window = nullptr;
        glfwTerminate();
        return false;
    }

    // ── Debug overlay ────────────────────────────────────────────────────────
    implementation_->lastFpsUpdateTime  = glfwGetTime();
    implementation_->lastFrameTime      = glfwGetTime();
    implementation_->renderedFrameCount = 0;
    implementation_->debugOverlay.Initialize();

    initialized_ = true;
    return true;
}

// ──────────────────────────────────────────────────────────────────────────────
// BeginFrame
// ──────────────────────────────────────────────────────────────────────────────
void OpenGLRenderer::BeginFrame() {
    // Reserved for future pre-frame GPU work (e.g. fence waits, UBO updates).
}

// ──────────────────────────────────────────────────────────────────────────────
// Render
// ──────────────────────────────────────────────────────────────────────────────
void OpenGLRenderer::Render(
    const SceneState&     state,
    const DebugState&     debugState,
    const audio::ExperienceSignals& signals,
    const ExperiencePreset& preset) {

    if (!initialized_) {
        return;
    }

    const double currentTime = glfwGetTime();
    const float deltaTime = static_cast<float>(currentTime - implementation_->lastFrameTime);
    implementation_->lastFrameTime = currentTime;

    auto& smoothed = implementation_->smoothedUniforms;

    // ── Audio-signal following ─────────────────────────────────────────────────
    // Low visual latency is a primary target, so transients use a fast attack
    // (~18 ms — an onset lands within ~2 frames) and a slower release (~110 ms)
    // to keep the decay smooth.  This keeps the image locked to the beat instead
    // of trailing it, while still avoiding per-frame strobing.
    constexpr float kAttack  = 55.0f; // rise time constant ~18 ms
    constexpr float kRelease = 9.0f;  // fall time constant ~110 ms

    // The preset scales how much energy drives brightness.
    const float targetEnergy = std::clamp(signals.energy * preset.energyMultiplier, 0.0f, 1.0f);
    smoothed.energy    = Follow(smoothed.energy,    targetEnergy,      deltaTime, kAttack, kRelease);
    smoothed.intensity = Follow(smoothed.intensity, signals.intensity, deltaTime, kAttack, kRelease);
    smoothed.bass      = Follow(smoothed.bass,      signals.bass,      deltaTime, kAttack, kRelease);
    smoothed.mid       = Follow(smoothed.mid,       signals.mid,       deltaTime, kAttack, kRelease);
    smoothed.treble    = Follow(smoothed.treble,    signals.treble,    deltaTime, kAttack, kRelease);

    // Beat pulse: instant on the beat frame (zero latency), then a fast decay so
    // it reads as a punch.  Peaks at the preset's beat response.
    if (signals.beat) {
        smoothed.beat = preset.beatResponse;
    } else {
        const float beatDecayRate = 10.0f; // exp(-dt * 10) gives ~37% after 100 ms
        smoothed.beat *= std::exp(-deltaTime * beatDecayRate);
    }

    // Mood is a slow scene property, not a transient — follow it gently.
    smoothed.mood = Follow(smoothed.mood, state.mood, deltaTime, 6.0f, 6.0f);

    // ── Preset palette & style ─────────────────────────────────────────────────
    // Every colour and style constant comes from the active preset — nothing is
    // hardcoded here.  The preset's transitionSpeed drives the easing rate, so
    // switching presets cross-fades the whole image instead of popping.  Each
    // update is a handful of float lerps on the persistent smoothed uniforms,
    // so a preset switch performs no heap allocation.
    const float presetAlpha =
        1.0f - std::exp(-deltaTime * std::max(preset.transitionSpeed, 0.0f));

    smoothed.colorLow    = LerpColor(smoothed.colorLow,    ToColor3(preset.colorLow),   presetAlpha);
    smoothed.colorMid    = LerpColor(smoothed.colorMid,    ToColor3(preset.colorMid),   presetAlpha);
    smoothed.colorHigh   = LerpColor(smoothed.colorHigh,   ToColor3(preset.colorHigh),  presetAlpha);
    smoothed.background  = LerpColor(smoothed.background,  ToColor3(preset.background), presetAlpha);
    smoothed.saturationBase  += (preset.saturationBase  - smoothed.saturationBase)  * presetAlpha;
    smoothed.saturationScale += (preset.saturationScale - smoothed.saturationScale) * presetAlpha;
    smoothed.motion          += (preset.motionIntensity - smoothed.motion)          * presetAlpha;
    smoothed.warp            += (preset.warp            - smoothed.warp)            * presetAlpha;
    smoothed.detail          += (preset.detail          - smoothed.detail)          * presetAlpha;

    // ── Signature form cross-fade ──────────────────────────────────────────────
    // The pattern is a discrete choice, so it can't be lerped like a colour.
    // Instead we snapshot the outgoing form and ease patternBlend 0 → 1; the
    // shader mixes the two forms while the blend runs, then draws only the active
    // form once it completes.  This is state juggling only — no allocation.
    const int incomingMode = static_cast<int>(preset.patternMode);
    if (!implementation_->patternInitialized) {
        implementation_->activePatternMode   = incomingMode;
        implementation_->previousPatternMode = incomingMode;
        implementation_->patternBlend        = 1.0f;
        implementation_->patternInitialized  = true;
    } else if (incomingMode != implementation_->activePatternMode) {
        implementation_->previousPatternMode = implementation_->activePatternMode;
        implementation_->activePatternMode   = incomingMode;
        implementation_->patternBlend        = 0.0f;
    }
    implementation_->patternBlend += (1.0f - implementation_->patternBlend) * presetAlpha;
    if (implementation_->patternBlend > 0.999f) {
        implementation_->patternBlend        = 1.0f;
        implementation_->previousPatternMode = implementation_->activePatternMode;
    }
    smoothed.patternMode         = implementation_->activePatternMode;
    smoothed.previousPatternMode = implementation_->previousPatternMode;
    smoothed.patternBlend        = implementation_->patternBlend;

    // Query framebuffer size for the uResolution uniform and viewport.
    int width  = 0;
    int height = 0;
    glfwGetFramebufferSize(implementation_->window, &width, &height);
    glViewport(0, 0, width, height);

    // Clear to black — the fullscreen triangle overwrites this, but the clear
    // is kept for correctness on framebuffers with a depth/stencil attachment.
    glClearColor(0.0F, 0.0F, 0.0F, 1.0F);
    glClear(GL_COLOR_BUFFER_BIT);

    // ── Fullscreen shader pass ────────────────────────────────────────────
    const float time = static_cast<float>(currentTime);

    shaderManager_.Bind();
    shaderManager_.SetUniforms(smoothed, time, width, height);

    glBindVertexArray(fullscreenVAO_);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0u);

    // ── Debug overlay (rendered on top, uses its own program internally) ──
    if (implementation_->showDebugOverlay) {
        DebugState stateWithFps = debugState;
        stateWithFps.fps = static_cast<float>(implementation_->currentFps);
        implementation_->debugOverlay.Render(stateWithFps, width, height);
    }
}

// ──────────────────────────────────────────────────────────────────────────────
// EndFrame
// ──────────────────────────────────────────────────────────────────────────────
bool OpenGLRenderer::EndFrame() {
    if (!initialized_) {
        return false;
    }

    glfwSwapBuffers(implementation_->window);
    glfwPollEvents();

    // ── Temporary preset controls (Stage 5.3) ─────────────────────────────────
    // F1..F6 request presets 0..5.  We latch the rising edge of the most recent
    // key here (events were just polled) and let the Runtime drain it via
    // ConsumePresetRequest, keeping preset ownership in the Runtime.
    constexpr int kPresetKeys[Implementation::kPresetKeyCount] = {
        GLFW_KEY_F1, GLFW_KEY_F2, GLFW_KEY_F3,
        GLFW_KEY_F4, GLFW_KEY_F5, GLFW_KEY_F6,
    };
    for (int i = 0; i < Implementation::kPresetKeyCount; ++i) {
        const bool pressed =
            glfwGetKey(implementation_->window, kPresetKeys[i]) == GLFW_PRESS;
        if (pressed && !implementation_->presetKeyWasPressed[i]) {
            implementation_->pendingPresetRequest = i;
        }
        implementation_->presetKeyWasPressed[i] = pressed;
    }

    // The grave/tilde (`) key toggles the debug overlay (F1 now selects a preset).
    const bool debugKeyIsPressed =
        glfwGetKey(implementation_->window, GLFW_KEY_GRAVE_ACCENT) == GLFW_PRESS;
    if (debugKeyIsPressed && !implementation_->debugKeyWasPressed) {
        implementation_->showDebugOverlay = !implementation_->showDebugOverlay;
    }
    implementation_->debugKeyWasPressed = debugKeyIsPressed;

    // FPS counter and window title update.
    ++implementation_->renderedFrameCount;
    const double currentTime  = glfwGetTime();
    const double elapsedTime  = currentTime - implementation_->lastFpsUpdateTime;
    if (elapsedTime >= 1.0) {
        implementation_->currentFps =
            static_cast<double>(implementation_->renderedFrameCount) / elapsedTime;

        std::ostringstream title;
        title << "PAPAGEDON Core v0.0.1 | FPS: "
              << std::lround(implementation_->currentFps);
        glfwSetWindowTitle(implementation_->window, title.str().c_str());

        implementation_->renderedFrameCount = 0;
        implementation_->lastFpsUpdateTime  = currentTime;
    }

    return glfwWindowShouldClose(implementation_->window) == GLFW_FALSE;
}

// ──────────────────────────────────────────────────────────────────────────────
// ConsumePresetRequest
// ──────────────────────────────────────────────────────────────────────────────
int OpenGLRenderer::ConsumePresetRequest() noexcept {
    if (implementation_ == nullptr) {
        return -1;
    }
    const int request = implementation_->pendingPresetRequest;
    implementation_->pendingPresetRequest = -1;
    return request;
}

// ──────────────────────────────────────────────────────────────────────────────
// Shutdown
// ──────────────────────────────────────────────────────────────────────────────
void OpenGLRenderer::Shutdown() noexcept {
    if (implementation_ == nullptr || !initialized_) {
        return;
    }

    implementation_->debugOverlay.Shutdown();

    shaderManager_.Shutdown();

    if (fullscreenVAO_ != 0u) {
        glDeleteVertexArrays(1, &fullscreenVAO_);
        fullscreenVAO_ = 0u;
    }

    glfwDestroyWindow(implementation_->window);
    implementation_->window = nullptr;
    glfwTerminate();
    initialized_ = false;
}

} // namespace papagedon
