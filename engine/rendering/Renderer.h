#pragma once

#include "IRenderer.h"

#include <memory>

namespace papagedon {

/// Backend-neutral renderer facade.
class Renderer final {
public:
    Renderer();
    ~Renderer();

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    bool Initialize();
    void BeginFrame();
    void Render();
    /// Returns false once the active backend requests application shutdown.
    [[nodiscard]] bool EndFrame();
    void Shutdown() noexcept;

private:
    std::unique_ptr<IRenderer> backend_;
};

} // namespace papagedon
