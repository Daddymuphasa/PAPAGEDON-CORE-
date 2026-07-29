#include <papagedon/core/Engine.h>

#include <AudioCapture.h>

#include <cstring>
#include <string>

// Usage:
//   papagedon-player [audio-file]      play a WAV/MP3 and visualise it
//   papagedon-player --list-audio      list input / output devices, then exit
//
// For a live set, set the audio source to a capture device instead of a file:
//   config/demo.json  "audioSource": "input", "captureDevice": <n>
//   or env            PAPAGEDON_AUDIO=input  PAPAGEDON_CAPTURE_DEVICE=<n>
int main(int argc, char** argv) {
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
        return 1;
    }

    engine.Run();
    engine.Shutdown();

    return 0;
}
