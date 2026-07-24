#include "OpenGLRenderer.h"
#include "../scene/SceneState.h"
#include "DebugOverlayRenderer.h"

#include <glad/glad.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <cmath>
#include <iomanip>
#include <memory>
#include <sstream>

namespace papagedon {

namespace {

struct Color { float r, g, b; };

Color GetProfileColor(const SceneProfile* profile) {
    if (!profile) {
        return {0.0F, 0.0F, 0.0F};
    }
    if (profile->sceneId == "calm") {
        return {20.0F / 255.0F, 40.0F / 255.0F, 90.0F / 255.0F};
    }
    if (profile->sceneId == "build-up") {
        return {255.0F / 255.0F, 140.0F / 255.0F, 0.0F / 255.0F};
    }
    if (profile->sceneId == "drop") {
        return {220.0F / 255.0F, 30.0F / 255.0F, 30.0F / 255.0F};
    }
    if (profile->sceneId == "ambient") {
        return {70.0F / 255.0F, 30.0F / 255.0F, 120.0F / 255.0F};
    }
    if (profile->sceneId == "silence") {
        return {0.0F, 0.0F, 0.0F};
    }
    return {0.0F, 0.0F, 0.0F};
}

} // namespace


class OpenGLRenderer::Implementation final {
public:
    GLFWwindow* window = nullptr;
    double lastFpsUpdateTime = 0.0;
    unsigned int renderedFrameCount = 0;
    double currentFps = 0.0;
    bool showDebugOverlay = false;
    bool f1WasPressed = false;
    DebugOverlayRenderer debugOverlay;
};

OpenGLRenderer::OpenGLRenderer(const bool vsyncEnabled)
    : implementation_{std::make_unique<Implementation>()},
      vsyncEnabled_{vsyncEnabled} {}

OpenGLRenderer::~OpenGLRenderer() {
    Shutdown();
}

bool OpenGLRenderer::Initialize() {
    if (initialized_) {
        return true;
    }

    if (glfwInit() != GLFW_TRUE) {
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);

    implementation_->window = glfwCreateWindow(
        1280,
        720,
        "PAPAGEDON Core v0.0.1",
        nullptr,
        nullptr);
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
    implementation_->lastFpsUpdateTime = glfwGetTime();
    implementation_->renderedFrameCount = 0;
    implementation_->debugOverlay.Initialize();
    initialized_ = true;
    return true;
}

void OpenGLRenderer::BeginFrame() {
    if (!initialized_) {
        return;
    }
}

void OpenGLRenderer::Render(const SceneState& state, const DebugState& debugState) {
    if (!initialized_) {
        return;
    }

    const Color c1 = GetProfileColor(state.previousProfile);
    const Color c2 = GetProfileColor(state.activeProfile);
    const float t = state.transitionProgress;

    const float r = c1.r + (c2.r - c1.r) * t;
    const float g = c1.g + (c2.g - c1.g) * t;
    const float b = c1.b + (c2.b - c1.b) * t;

    glClearColor(r, g, b, 1.0F);
    glClear(GL_COLOR_BUFFER_BIT);

    // Mesh, shader, UI, and Scene DNA rendering are intentionally deferred.

    if (implementation_->showDebugOverlay) {
        int width, height;
        glfwGetFramebufferSize(implementation_->window, &width, &height);
        
        DebugState stateWithFps = debugState;
        stateWithFps.fps = static_cast<float>(implementation_->currentFps);
        implementation_->debugOverlay.Render(stateWithFps, width, height);
    }
}

bool OpenGLRenderer::EndFrame() {
    if (!initialized_) {
        return false;
    }

    glfwSwapBuffers(implementation_->window);
    glfwPollEvents();

    bool f1IsPressed = glfwGetKey(implementation_->window, GLFW_KEY_F1) == GLFW_PRESS;
    if (f1IsPressed && !implementation_->f1WasPressed) {
        implementation_->showDebugOverlay = !implementation_->showDebugOverlay;
    }
    implementation_->f1WasPressed = f1IsPressed;

    ++implementation_->renderedFrameCount;
    const double currentTime = glfwGetTime();
    const double elapsedTime = currentTime - implementation_->lastFpsUpdateTime;
    if (elapsedTime >= 1.0) {
        implementation_->currentFps = static_cast<double>(implementation_->renderedFrameCount) / elapsedTime;
        std::ostringstream title;
        title << "PAPAGEDON Core v0.0.1 | FPS: "
              << std::lround(implementation_->currentFps);
        glfwSetWindowTitle(implementation_->window, title.str().c_str());
        implementation_->renderedFrameCount = 0;
        implementation_->lastFpsUpdateTime = currentTime;
    }

    return glfwWindowShouldClose(implementation_->window) == GLFW_FALSE;
}

void OpenGLRenderer::Shutdown() noexcept {
    if (implementation_ == nullptr || !initialized_) {
        return;
    }

    implementation_->debugOverlay.Shutdown();
    glfwDestroyWindow(implementation_->window);
    implementation_->window = nullptr;
    glfwTerminate();
    initialized_ = false;
}

} // namespace papagedon
