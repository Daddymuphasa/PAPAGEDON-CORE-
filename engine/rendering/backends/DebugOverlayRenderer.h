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

    /// Draws a clean, minimal loading frame (dim gold wordmark) while the engine
    /// initializes.  The animated cinematic logo intro plays afterwards.
    void RenderSplash(const char* version, const char* status, float progress,
                      int windowWidth, int windowHeight);

    /// Draws a short centred text toast near the bottom of the frame — used to
    /// flash the active shader name when the operator switches shaders live.
    void RenderToast(const char* text, int windowWidth, int windowHeight);

    /// Draws a compact input-level meter (bass / mid / treble / energy bars + a
    /// signal indicator) for confirming live audio during soundcheck.
    void RenderMeter(const DebugState& state, int windowWidth, int windowHeight);

    /// Draws the letter-spaced gold "PAPAGEDON" wordmark, centred — used by the
    /// cinematic startup intro.  alpha fades it in / out.
    void RenderBrandWordmark(int windowWidth, int windowHeight, float alpha);

    /// Draws the full live-controls menu (all keys + features), gold headings on
    /// warm-white entries, with a drop shadow for legibility.  alpha fades it.
    void RenderMenu(int windowWidth, int windowHeight, float alpha);

    void Shutdown() noexcept;

private:
    // Draws text at pixel (x, y) with the given colour and alpha (defaults white).
    void RenderText(const char* text, float x, float y, float scale,
                    int windowWidth, int windowHeight,
                    float r = 1.0F, float g = 1.0F, float b = 1.0F, float a = 1.0F);
    // Same, but draws a dark drop-shadow copy first for legibility over visuals.
    void RenderTextShadowed(const char* text, float x, float y, float scale,
                            int windowWidth, int windowHeight,
                            float r, float g, float b, float a);

    unsigned int shaderProgram_ = 0;
    unsigned int vao_ = 0;
    unsigned int vbo_ = 0;
    unsigned int texture_ = 0;
    int          colorLoc_ = -1;
    int          alphaLoc_ = -1;
    bool initialized_ = false;
};

} // namespace papagedon
