#pragma once

#include <papagedon/runtime/Runtime.h>
#include <papagedon/utilities/Logger.h>

namespace papagedon::core {

class Engine final {
public:
    Engine();
    ~Engine();

    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;

    bool Initialize();
    void Run();
    void Shutdown() noexcept;
    void RequestStop() noexcept;

private:
    utilities::Logger logger_;
    runtime::Runtime runtime_;
    bool initialized_ = false;
};

} // namespace papagedon::core
