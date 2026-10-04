#pragma once

#include <papagedon/runtime/Runtime.h>
#include <papagedon/utilities/Logger.h>

#include <string>

namespace papagedon::core {

class Engine final {
public:
    Engine();
    ~Engine();

    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;

    /// Initializes the engine. An optional audio-file path selects the clip to
    /// play; when empty the runtime falls back to its default (test.mp3).
    bool Initialize(const std::string& audioPath = {});
    void Run();
    void Shutdown() noexcept;
    void RequestStop() noexcept;

private:
    utilities::Logger logger_;
    runtime::Runtime runtime_;
    bool initialized_ = false;
};

} // namespace papagedon::core
