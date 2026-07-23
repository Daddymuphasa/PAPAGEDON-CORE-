#pragma once

#include <SceneState.h>

#include <chrono>
#include <cstdint>

namespace papagedon {

/// Per-frame data produced by the renderer.
struct RenderFrame final {
    std::uint64_t frameNumber = 0;
    std::chrono::duration<double> deltaTime{};
    std::chrono::steady_clock::time_point timestamp{};
    SceneState activeScene{};
    std::chrono::duration<double> renderTime{};
};

} // namespace papagedon
