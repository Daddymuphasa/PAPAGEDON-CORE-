#pragma once

#include "../../rendering/DebugState.h"

namespace papagedon {

class DebugOverlayRenderer final {
public:
    DebugOverlayRenderer();
    ~DebugOverlayRenderer();

    DebugOverlayRenderer(const DebugOverlayRenderer&) = delete;
    DebugOverlayRenderer& operator=(const DebugOverlayRenderer&) = delete;

    bool Initialize();
    void Render(const DebugState& state, int windowWidth, int windowHeight);
    void Shutdown() noexcept;

private:
    void RenderText(const char* text, float x, float y, float scale, int windowWidth, int windowHeight);

    unsigned int shaderProgram_ = 0;
    unsigned int vao_ = 0;
    unsigned int vbo_ = 0;
    unsigned int texture_ = 0;
    bool initialized_ = false;
};

} // namespace papagedon
