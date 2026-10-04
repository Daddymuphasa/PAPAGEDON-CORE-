#include <papagedon/core/Engine.h>

#include <string>

// Usage: papagedon-player [audio-file]
// With no argument the engine falls back to its default clip (test.mp3).
int main(int argc, char** argv) {
    papagedon::core::Engine engine;

    const std::string audioPath = (argc > 1) ? std::string{argv[1]} : std::string{};

    if (!engine.Initialize(audioPath)) {
        return 1;
    }

    engine.Run();
    engine.Shutdown();

    return 0;
}
