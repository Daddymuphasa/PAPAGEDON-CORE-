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

    /// Draws the startup splash: logo, engine version, a status line, and a text
    /// progress bar (progress in [0, 1]).  Used while the engine initializes.
    void RenderSplash(const char* version, const char* status, float progress,
                      int windowWidth, int windowHeight);

    /// Draws a short centred text toast near the bottom of the frame — used to
    /// flash the active shader name when the operator switches shaders live.
    void RenderToast(const char* text, int windowWidth, int windowHeight);

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
