#pragma once

#include <atomic>
#include <chrono>

namespace papagedon::utilities {
class Logger;
}

namespace papagedon::runtime {

class Runtime final {
public:
    using FrameDuration = std::chrono::duration<double>;

    explicit Runtime(utilities::Logger& logger) noexcept;

    Runtime(const Runtime&) = delete;
    Runtime& operator=(const Runtime&) = delete;

    bool Initialize();
    void Run();
    void Shutdown() noexcept;
    void RequestStop() noexcept;

    [[nodiscard]] bool IsRunning() const noexcept;

private:
    void Update(FrameDuration deltaTime) noexcept;

    utilities::Logger& logger_;
    std::atomic_bool running_{false};
    bool initialized_ = false;
};

} // namespace papagedon::runtime
