#include "OpenGLRenderer.h"

#include <glad/glad.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <cmath>
#include <iomanip>
#include <memory>
#include <sstream>

namespace papagedon {

class OpenGLRenderer::Implementation final {
public:
    GLFWwindow* window = nullptr;
    double lastFpsUpdateTime = 0.0;
    unsigned int renderedFrameCount = 0;
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
    initialized_ = true;
    return true;
}

void OpenGLRenderer::BeginFrame() {
    if (!initialized_) {
        return;
    }

    glClearColor(0.0F, 0.0F, 0.0F, 1.0F);
    glClear(GL_COLOR_BUFFER_BIT);
}

void OpenGLRenderer::Render() {
    // Mesh, shader, UI, and Scene DNA rendering are intentionally deferred.
}

bool OpenGLRenderer::EndFrame() {
    if (!initialized_) {
        return false;
    }

    glfwSwapBuffers(implementation_->window);
    glfwPollEvents();

    ++implementation_->renderedFrameCount;
    const double currentTime = glfwGetTime();
    const double elapsedTime = currentTime - implementation_->lastFpsUpdateTime;
    if (elapsedTime >= 1.0) {
        std::ostringstream title;
        title << "PAPAGEDON Core v0.0.1 | FPS: "
              << std::lround(static_cast<double>(implementation_->renderedFrameCount) / elapsedTime);
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

    glfwDestroyWindow(implementation_->window);
    implementation_->window = nullptr;
    glfwTerminate();
    initialized_ = false;
}

} // namespace papagedon
