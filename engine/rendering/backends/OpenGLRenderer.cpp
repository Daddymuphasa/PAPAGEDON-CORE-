#include "OpenGLRenderer.h"
#include "../scene/SceneState.h"
#include "../ShaderUniforms.h"
#include "DebugOverlayRenderer.h"

#include <glad/glad.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <iomanip>
#include <memory>
#include <sstream>
#include <string>

namespace papagedon {

// ──────────────────────────────────────────────────────────────────────────────
// Scene-palette helpers
//
// Scene profiles store their palette as "#RRGGBB" strings.  These free functions
// turn that data-only description into GPU-ready colours.  Parsing is tolerant:
// any malformed entry yields a neutral grey so a bad palette never breaks output.
// ──────────────────────────────────────────────────────────────────────────────
namespace {

Color3 ParseHexColor(const std::string& hex) noexcept {
    const auto nibble = [](const char c) -> int {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        return -1;
    };

    const std::size_t offset = (!hex.empty() && hex.front() == '#') ? 1u : 0u;
    if (hex.size() < offset + 6u) {
        return Color3{0.5F, 0.5F, 0.5F};
    }

    std::array<int, 6> digits{};
    for (std::size_t i = 0; i < digits.size(); ++i) {
        digits[i] = nibble(hex[offset + i]);
        if (digits[i] < 0) {
            return Color3{0.5F, 0.5F, 0.5F};
        }
    }

    return Color3{
        static_cast<float>(digits[0] * 16 + digits[1]) / 255.0F,
        static_cast<float>(digits[2] * 16 + digits[3]) / 255.0F,
        static_cast<float>(digits[4] * 16 + digits[5]) / 255.0F,
    };
}

// Resolve a profile's palette into exactly three ramp stops, tolerating lists
// with fewer or more than three entries.
std::array<Color3, 3> PaletteFromProfile(const SceneProfile* const profile) noexcept {
    std::array<Color3, 3> stops{
        Color3{0.05F, 0.05F, 0.05F},
        Color3{0.30F, 0.30F, 0.30F},
        Color3{0.85F, 0.85F, 0.85F},
    };
    if (profile == nullptr || profile->colorPalette.empty()) {
        return stops;
    }

    const auto& palette = profile->colorPalette;
    for (std::size_t i = 0; i < stops.size(); ++i) {
        // Clamp to the last colour when the palette has fewer than three entries.
        const std::size_t src = std::min(i, palette.size() - 1u);
        stops[i] = ParseHexColor(palette[src]);
    }
    return stops;
}

Color3 LerpColor(const Color3& a, const Color3& b, const float t) noexcept {
    return Color3{
        a.r + (b.r - a.r) * t,
        a.g + (b.g - a.g) * t,
        a.b + (b.b - a.b) * t,
    };
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
    bool        f1WasPressed        = false;
    DebugOverlayRenderer debugOverlay;
    ShaderUniforms smoothedUniforms;

    // Cached palette parse — re-parsed only when the scene profile pointer
    // changes, keeping the per-frame path free of string work.
    const SceneProfile*   cachedActiveProfile   = nullptr;
    const SceneProfile*   cachedPreviousProfile = nullptr;
    std::array<Color3, 3> activePalette{};
    std::array<Color3, 3> previousPalette{};
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
    const audio::ExperienceSignals& signals) {

    if (!initialized_) {
        return;
    }

    const double currentTime = glfwGetTime();
    const float deltaTime = static_cast<float>(currentTime - implementation_->lastFrameTime);
    implementation_->lastFrameTime = currentTime;

    // ── Signal Smoothing ──────────────────────────────────────────────────────
    const float smoothingRate = 12.0f; // Tunable parameter
    const float alpha = 1.0f - std::exp(-deltaTime * smoothingRate);

    auto& smoothed = implementation_->smoothedUniforms;
    smoothed.energy    += (signals.energy - smoothed.energy) * alpha;
    smoothed.intensity += (signals.intensity - smoothed.intensity) * alpha;
    smoothed.bass      += (signals.bass - smoothed.bass) * alpha;
    smoothed.mid       += (signals.mid - smoothed.mid) * alpha;
    smoothed.treble    += (signals.treble - smoothed.treble) * alpha;

    // Beat pulse with ~100ms decay
    if (signals.beat) {
        smoothed.beat = 1.0f;
    } else {
        const float beatDecayRate = 10.0f; // exp(-dt * 10) gives ~37% after 100ms
        smoothed.beat *= std::exp(-deltaTime * beatDecayRate);
    }

    smoothed.mood += (state.mood - smoothed.mood) * alpha;

    // ── Scene palette ─────────────────────────────────────────────────────────
    // Re-parse the hex palettes only when a profile pointer changes; every other
    // frame the work is three colour lerps.  SceneState::transitionProgress drives
    // the cross-fade from the outgoing scene's palette to the incoming one, so
    // scene changes are visible on screen for the first time.
    if (state.activeProfile != implementation_->cachedActiveProfile) {
        implementation_->cachedActiveProfile = state.activeProfile;
        implementation_->activePalette = PaletteFromProfile(state.activeProfile);
    }
    // Fall back to the active profile when no previous one exists (first scene),
    // which makes the blend below a no-op at transitionProgress == 1.
    const SceneProfile* const previousProfile =
        state.previousProfile != nullptr ? state.previousProfile : state.activeProfile;
    if (previousProfile != implementation_->cachedPreviousProfile) {
        implementation_->cachedPreviousProfile = previousProfile;
        implementation_->previousPalette = PaletteFromProfile(previousProfile);
    }

    const float blend = std::clamp(state.transitionProgress, 0.0f, 1.0f);
    smoothed.colorLow  = LerpColor(implementation_->previousPalette[0],
                                   implementation_->activePalette[0], blend);
    smoothed.colorMid  = LerpColor(implementation_->previousPalette[1],
                                   implementation_->activePalette[1], blend);
    smoothed.colorHigh = LerpColor(implementation_->previousPalette[2],
                                   implementation_->activePalette[2], blend);

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

    // F1 toggles the debug overlay.
    const bool f1IsPressed =
        glfwGetKey(implementation_->window, GLFW_KEY_F1) == GLFW_PRESS;
    if (f1IsPressed && !implementation_->f1WasPressed) {
        implementation_->showDebugOverlay = !implementation_->showDebugOverlay;
    }
    implementation_->f1WasPressed = f1IsPressed;

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
