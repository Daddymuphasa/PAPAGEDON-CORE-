#include <papagedon/core/Engine.h>

#include <AudioCapture.h>

#include <cstdio>
#include <cstring>
#include <exception>
#include <string>

// Usage:
//   papagedon-player [audio-file]      play a WAV/MP3 and visualise it
//   papagedon-player --list-audio      list input / output devices, then exit
//
// For a live set, set the audio source to a capture device instead of a file:
//   config/demo.json  "audioSource": "input", "captureDevice": <n>
//   or env            PAPAGEDON_AUDIO=input  PAPAGEDON_CAPTURE_DEVICE=<n>
int main(int argc, char** argv) {
    try {
        for (int i = 1; i < argc; ++i) {
            if (std::strcmp(argv[i], "--list-audio") == 0) {
                papagedon::audio::AudioCapture::ListDevices();
                return 0;
            }
        }

        papagedon::core::Engine engine;

        // First non-flag argument is an audio file to play (file mode).
        const std::string audioPath =
            (argc > 1 && argv[1][0] != '-') ? std::string{argv[1]} : std::string{};

        if (!engine.Initialize(audioPath)) {
            std::fprintf(stderr,
                "\n  PAPAGEDON could not start.\n"
                "  Check that your GPU driver is up to date and supports OpenGL 3.3+.\n\n");
            return 1;
        }

        engine.Run();
        engine.Shutdown();

        return 0;
    } catch (const std::exception& e) {
        std::fprintf(stderr,
            "\n  PAPAGEDON crashed: %s\n"
            "  Please report this issue.\n\n", e.what());
        return 1;
    } catch (...) {
        std::fprintf(stderr,
            "\n  PAPAGEDON crashed (unknown error).\n"
            "  Please report this issue.\n\n");
        return 1;
    }
}
