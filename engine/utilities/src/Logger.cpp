#include <papagedon/utilities/Logger.h>

#include <iostream>
#include <mutex>

namespace papagedon::utilities {
void Logger::INFO(const std::string_view message) const {
    Log(Level::Info, message);
}

void Logger::WARNING(const std::string_view message) const {
    Log(Level::Warning, message);
}

void Logger::ERROR(const std::string_view message) const {
    Log(Level::Error, message);
}

void Logger::Log(const Level level, const std::string_view message) const {
    const auto levelName = [](const Level currentLevel) -> std::string_view {
        switch (currentLevel) {
        case Level::Info:
            return "INFO";
        case Level::Warning:
            return "WARNING";
        case Level::Error:
            return "ERROR";
        }

        return "UNKNOWN";
    };

    const std::lock_guard lock{outputMutex_};
    std::clog << '[' << levelName(level) << "] " << message << '\n';
}

} // namespace papagedon::utilities
