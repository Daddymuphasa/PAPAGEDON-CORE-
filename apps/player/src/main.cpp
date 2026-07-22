#include <papagedon/core/Engine.h>

int main() {
    papagedon::core::Engine engine;

    if (!engine.Initialize()) {
        return 1;
    }

    engine.Run();
    engine.Shutdown();

    return 0;
}
