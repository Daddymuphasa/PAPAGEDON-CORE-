#pragma once

#include <SceneState.h>

namespace papagedon {

/// Interface implemented by each graphics backend.
class IRenderer {
public:
    virtual ~IRenderer() = default;

    virtual bool Initialize() = 0;
    virtual void BeginFrame() = 0;
    virtual void Render(const SceneState& sceneState) = 0;
    virtual void EndFrame() = 0;
    virtual void Shutdown() noexcept = 0;
};

} // namespace papagedon
