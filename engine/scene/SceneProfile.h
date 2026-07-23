#pragma once

#include <chrono>
#include <string>
#include <vector>

namespace papagedon {

/// Data-only description of the intended character of a scene.
///
/// Scene profiles deliberately contain no renderer, asset, or platform state.
struct SceneProfile final {
    std::string sceneId;
    std::string name;
    std::string environmentType;
    std::string mood;
    std::vector<std::string> colorPalette;
    std::string lightingStyle;
    std::string weatherType;
    std::string cameraStyle;
    std::string particleStyle;
    std::string atmosphere;
    std::chrono::milliseconds transitionDuration{0};
};

} // namespace papagedon
