#pragma once

#include <atomic>
#include <chrono>

#include <AudioAnalyzer.h>
#include <AudioInput.h>
#include <ExperienceGraph.h>

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
    audio::AudioAnalyzer audioAnalyzer_;
    audio::AudioInput audioInput_{};
    papagedon::ExperienceGraph experienceGraph_;
    std::atomic_bool running_{false};
    bool initialized_ = false;
    float lastLoggedBpm_ = 0.0F;
    bool lastBeatLogged_ = false;
    std::uint64_t debugFrameCounter_ = 0;
};

} // namespace papagedon::runtime
