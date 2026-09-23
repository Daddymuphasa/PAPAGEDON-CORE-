#pragma once

namespace papagedon::pgx {

/// Feedback controls for a native PGX visual.
///
/// These values describe how much visual memory a preset wants the renderer to
/// preserve between frames.  They are renderer-neutral: an OpenGL backend may
/// implement them with ping-pong FBOs, while a future WebGPU backend can map the
/// same data to its own pass graph.
struct FeedbackSettings final {
    /// Previous-frame retention [0 = no memory, 1 = very persistent trails].
    float decay = 0.0F;

    /// Previous-frame zoom drift.  Values above 1.0 push the image outward.
    float zoom = 1.0F;

    /// Previous-frame rotation drift in radians per second.
    float rotation = 0.0F;

    /// Procedural warp strength applied to the feedback pass.
    float warp = 0.0F;

    /// Extra feedback displacement on beat pulses.
    float beatWarp = 0.0F;
};

enum class WaveformMode : int {
    None = 0,
    Line,
    Ribbon,
    Ring,
};

/// Built-in PGX audio geometry layer.
///
/// The first runtime implementation can render this directly from waveform or
/// FFT buffers.  Presets declare the desired layer here instead of hardcoding it
/// inside fragment shaders.
struct WaveformLayer final {
    WaveformMode mode = WaveformMode::None;
    float opacity = 0.0F;
    float thickness = 1.0F;
    float radius = 0.45F;
    float bassResponse = 0.0F;
    float trebleResponse = 0.0F;
};

/// Native PAPAGEDON runtime description attached to an ExperiencePreset.
struct RuntimeSettings final {
    FeedbackSettings feedback{};
    WaveformLayer waveform{};
};

} // namespace papagedon::pgx
