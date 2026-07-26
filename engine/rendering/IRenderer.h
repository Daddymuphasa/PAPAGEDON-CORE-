#pragma once

#include "ShaderUniforms.h"

namespace papagedon {

struct SceneState;
struct DebugState;

/// Interface implemented by each graphics backend.
class IRenderer {
public:
    virtual ~IRenderer() = default;

    virtual bool Initialize() = 0;
    virtual void BeginFrame() = 0;
    virtual void Render(
        const SceneState&    state,
        const DebugState&    debugState,
        const ShaderUniforms& uniforms) = 0;
    /// Returns false once the backend has received a request to close.
    virtual bool EndFrame() = 0;
    virtual void Shutdown() noexcept = 0;
};

} // namespace papagedon
