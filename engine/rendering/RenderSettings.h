#pragma once

#include <array>
#include <cstdint>

namespace papagedon {

/// Presentation settings for the active rendering backend.
struct RenderSettings final {
    std::uint32_t targetFPS = 120;
    std::array<float, 4> clearColor{0.0F, 0.0F, 0.0F, 1.0F};
    bool vsyncEnabled = true;
    float renderScale = 1.0F;
    bool debugMode = false;
};

} // namespace papagedon
