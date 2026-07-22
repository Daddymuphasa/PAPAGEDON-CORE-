#pragma once

#include <mutex>
#include <string_view>

namespace papagedon::utilities {

class Logger final {
public:
    Logger() = default;

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    void INFO(std::string_view message) const;
    void WARNING(std::string_view message) const;
    void ERROR(std::string_view message) const;

private:
    enum class Level {
        Info,
        Warning,
        Error,
    };

    void Log(Level level, std::string_view message) const;

    mutable std::mutex outputMutex_;
};

} // namespace papagedon::utilities
