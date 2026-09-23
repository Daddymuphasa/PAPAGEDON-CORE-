# PAPAGEDON Core

PAPAGEDON is a C++20 real-time engine for AI-powered immersive music
experiences.  The goal is not to be a simple music visualizer.  The goal is to
become a runtime layer for adaptive audiovisual worlds across desktop, web,
mobile, XR, live events, LED walls, projection mapping and future platforms.

## Runtime Direction

The native visual runtime direction is **PGX**: PAPAGEDON's own preset and
render-pass model.  PGX is designed to take the strongest ideas from systems
like MilkDrop/projectM, while staying efficient, modular and owned by the
PAPAGEDON architecture.

Core PGX ideas:

- Native preset data, not `.milk` compatibility as the foundation.
- Feedback buffers and visual memory.
- Audio-reactive render passes.
- Waveform and spectrum geometry layers.
- Scene/director metadata for intelligent visual pacing.
- Future AI generation into a validated PGX/shader pipeline.

MilkDrop/projectM compatibility may become a future plugin, but it is not the
core runtime path.

## Recent PGX Runtime Work

The engine now has the first native PGX visual-memory foundation.

### Added PGX Data Contracts

New PGX runtime types were added under:

```text
engine/experience/pgx/PgxTypes.h
```

They define:

- `FeedbackSettings`
- `WaveformMode`
- `WaveformLayer`
- `RuntimeSettings`

`ExperiencePreset` now carries `pgx::RuntimeSettings`, allowing each preset to
describe how much visual memory, warp, beat displacement and audio geometry it
wants.

### Updated Built-In Presets

All built-in presets now include PGX feedback and waveform defaults.

Examples:

- Ambient presets use longer decay and softer drift.
- High-impact presets use stronger beat warp.
- Spectrum/ring-style presets request clearer audio geometry.

### Extended Shader Uniforms

`ShaderUniforms` and `ShaderManager` now pass PGX runtime controls to shaders:

- feedback decay
- feedback zoom
- feedback rotation
- feedback warp
- beat-driven feedback warp
- waveform mode
- waveform opacity/thickness/radius
- waveform bass/treble response

### Added OpenGL PGX Feedback Pipeline

The OpenGL backend now renders visuals through a visual-memory loop:

```text
Live visual shader
-> offscreen scene texture
-> PGX feedback composite pass
-> ping-pong feedback textures
-> screen
```

This means PAPAGEDON no longer has to redraw visuals from zero every frame.  The
previous composited frame can be warped, decayed and blended into the next frame,
creating the foundation for living, evolving visuals.

### Added Lightweight Waveform Preview

The default shader now previews PGX waveform intent with line/ribbon/ring-style
audio-reactive overlays.  This is a temporary preview layer; the long-term path
is a dedicated waveform/spectrum render pass driven by real audio buffers.

## Build

From the repository root:

```powershell
cmake --build build
```

Current verification status:

```text
[100%] Built target papagedon-player
```

No CTest tests are currently registered in the build.

## Next Technical Priorities

1. Add real audio textures:
   - waveform sample buffer
   - 64-band spectrum texture
   - instant and smoothed audio features

2. Replace waveform preview with a dedicated PGX waveform pass:
   - oscilloscope line
   - radial spectrum
   - ribbon modes
   - additive blending

3. Grow PGX into a pass graph:
   - base pass
   - feedback/warp pass
   - waveform pass
   - shape pass
   - composite pass
   - transition pass

4. Add AI generation later, after PGX is strong:
   - AI generates PGX presets or shader modules.
   - Generated content is validated and compiled before hot-loading.
   - The real-time renderer stays fast and stable.
