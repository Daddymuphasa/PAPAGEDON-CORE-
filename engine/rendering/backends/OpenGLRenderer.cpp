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

namespace papagedon {

// ──────────────────────────────────────────────────────────────────────────────
// Implementation (pimpl)
// ──────────────────────────────────────────────────────────────────────────────
class OpenGLRenderer::Implementation final {
public:
    GLFWwindow* window              = nullptr;
    double      lastFpsUpdateTime   = 0.0;
    unsigned int renderedFrameCount = 0;
    double      currentFps          = 0.0;
    bool        showDebugOverlay    = false;
    bool        f1WasPressed        = false;
    DebugOverlayRenderer debugOverlay;
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
    const SceneState&     /*state*/,
    const DebugState&     debugState,
    const ShaderUniforms& uniforms) {

    if (!initialized_) {
        return;
    }

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
    const float time = static_cast<float>(glfwGetTime());

    shaderManager_.Bind();
    shaderManager_.SetUniforms(uniforms, time, width, height);

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
