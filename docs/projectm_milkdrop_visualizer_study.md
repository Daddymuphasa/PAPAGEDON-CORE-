# projectM / MilkDrop Study for PAPAGEDON

Date: 2026-09-23

## Why projectM feels better

projectM is not just a folder of fragment shaders. Its core library, libprojectM,
parses MilkDrop presets, analyzes PCM audio with FFT and beat detection, applies
preset logic to those audio features, and renders the result through OpenGL.

MilkDrop presets are small visual programs stored as `.milk` INI-style files.
They commonly combine:

- Global visual parameters such as decay, gamma, zoom, rotation, warp, borders,
  waveform modes, and motion settings.
- Per-frame equations that update visual state once each frame.
- Per-pixel or per-vertex equations that warp the previous frame across a mesh.
- Custom waveform blocks that draw audio wave/spectrum geometry.
- Custom shape blocks that draw independent layered shapes.
- Optional warp and composite shader code in newer presets.
- Previous-frame feedback, where the last rendered image is decayed, warped,
  recolored, and composited into the next frame.

That feedback-and-warp loop is the secret sauce. The screen is not redrawn from
zero every frame. It is continuously remixed.

## Language and technology

projectM / libprojectM is primarily C++ and renders with OpenGL / OpenGL ES. The
SDL frontend we downloaded is a C++ SDL application using libprojectM. MilkDrop
itself originated as a Winamp visualizer; modern projectM implements compatible
preset behavior cross-platform.

MilkDrop preset authoring is not ordinary GLSL-only shader authoring. It is a
domain-specific visual system:

- `.milk` preset files are text.
- Equations drive parameters like `zoom`, `rot`, `warp`, `wave_r`, `wave_a`,
  `q1..q32`, etc.
- Newer presets may include HLSL-style warp/composite shader blocks.
- projectM translates or adapts this model into an OpenGL rendering path.

## What PAPAGEDON currently has

PAPAGEDON already has the beginning of a serious system:

- C++20 engine structure.
- Audio capture/playback/analyzer.
- FFT-based bass/mid/treble/energy/beat/BPM signals.
- Experience graph and auto-director.
- OpenGL fullscreen shader rendering.
- Shader packs with metadata.
- Dramatic shader-to-shader transition FBOs.
- Theme/palette controls.
- Debug overlay and live controls.

But the visual renderer currently behaves mostly like:

Audio signals -> smoothed uniforms -> one fullscreen GLSL shader -> screen

That can look good, but it will plateau quickly. It does not yet have MilkDrop's
living memory: previous-frame decay, feedback warping, layered waves/shapes, or
preset-local state/equations.

## Main gap

PAPAGEDON has visual forms. MilkDrop/projectM has a visual runtime.

The difference:

- PAPAGEDON shaders are mostly stateless procedural scenes.
- MilkDrop presets are stateful visual compositions.
- PAPAGEDON switches between visual files.
- MilkDrop mutates, warps, decays, overlays, and evolves the image continuously.
- PAPAGEDON has broad audio bands.
- MilkDrop exposes enough audio variables to let preset authors build behavior.

## PGX, not `.milk`, is the core path

PAPAGEDON should not build its core around importing MilkDrop presets.  `.milk`
compatibility can be a future plugin for migration, comparison or community
onboarding, but the efficient runtime path should be PGX: PAPAGEDON's own native
visual experience format.

PGX should express the strongest MilkDrop/projectM concepts in cleaner engine
terms:

- feedback settings
- render passes
- audio geometry layers
- preset-local state
- director metadata
- semantic audio mappings

The implementation started by adding native PGX runtime data to
`ExperiencePreset`: feedback controls and waveform layer intent.  The renderer
now receives those controls through `ShaderUniforms`.

The first OpenGL PGX feedback pass is now in place: PAPAGEDON renders the live
visual into an offscreen scene texture, composites it with a warped/decayed
previous-frame texture, stores the result in ping-pong feedback buffers, then
presents the composited frame to screen.  This is the first real visual-memory
loop in the engine and the foundation for stronger PGX pass-graph work.

## What to copy conceptually, not literally

Do not copy projectM code directly into the engine without a license decision.
Instead, copy the architecture ideas in PAPAGEDON's own terms.

### 1. Add a feedback render pipeline

Add ping-pong framebuffers:

- `previousFrame`
- `currentFrame`
- optional `feedbackFrame`

Render order:

1. Warp previous frame into current frame.
2. Decay/fade it using preset settings.
3. Draw reactive waves/shapes/particles on top.
4. Apply final composite pass.
5. Swap buffers.

This is the single biggest visual upgrade.

### 2. Split shaders into passes

Move from one fullscreen fragment shader to a small pass graph:

- `WarpPass`
- `BasePass`
- `WaveformPass`
- `ShapePass`
- `CompositePass`
- `TransitionPass`

This maps cleanly to the existing renderer without destroying modularity.

### 3. Expand audio features

Current signals are good but too coarse. Add:

- `bassAttack`, `midAttack`, `trebleAttack`
- smoothed and instant values for each band
- waveform samples for drawing custom waves
- spectrum texture or fixed band array, e.g. 64 bands
- onset strength
- beat phase
- confidence per feature

MilkDrop-style visuals need both slow musical context and fast transients.

### 4. Introduce PAPAGEDON preset state

Each preset should own small persistent state:

- timers
- random seeds
- q-style scratch values
- last beat time
- accumulated phase
- motion parameters

This can be data-driven without copying `.milk` exactly.

### 5. Add waveform and shape layers

MilkDrop often wins because it has obvious music geometry over abstract motion.
PAPAGEDON needs GPU-rendered layers:

- oscilloscope line
- circular spectrum
- ribbon waveform
- radial bars
- beat rings
- additive custom shapes

These should be renderer primitives, not hardcoded inside one shader.

### 6. Add a preset metadata scoring model

Our metadata is a good start. Make it richer:

- energy range
- BPM range
- bass/mid/treble affinities
- motion density
- color warmth
- peak suitability
- transition style
- visual risk/performance cost

Then the AutoDirector can select visuals more like a VJ.

### 7. Optional: build a `.milk` importer as a plugin later

Do this after the native PAPAGEDON runtime exists.

MVP importer:

- parse general numeric parameters
- parse basic wave/shape settings
- ignore arbitrary equations at first
- map decay/zoom/rot/warp into PAPAGEDON feedback uniforms

Full importer:

- expression parser/evaluator
- q-variable support
- custom waveform/shape equations
- HLSL-to-GLSL or restricted shader translation

This should stay outside the core because GPL/preset compatibility/licensing
concerns should not infect the PGX runtime by accident.

## Immediate implementation plan

### Phase 1: Feedback foundation

Create renderer-owned ping-pong FBOs and a feedback composite shader.

New concepts:

- `FeedbackSettings`
- `FeedbackPass`
- `uPreviousFrame`
- `uDecay`
- `uZoom`
- `uRotation`
- `uWarp`
- `uBeatWarp`

Expected result: visuals stop feeling like flat procedural loops and start
breathing/melting/evolving like projectM.

### Phase 2: Audio texture / spectrum bands

Expose a compact GPU audio texture or uniform array:

- 64 FFT bands
- waveform history
- smoothed + instant band values

Expected result: shader authors can draw real music structure instead of only
using `bass`, `mid`, and `treble`.

### Phase 3: Waveform layer

Add a simple OpenGL waveform renderer:

- line strip
- additive blending
- theme colors
- circular/ribbon modes

Expected result: the viewer sees the music, not just inferred motion.

### Phase 4: Preset runtime state

Add a native PAPAGEDON preset state object:

- persistent values
- random seeds
- beat phase
- section phase
- transition phase

Expected result: visuals can evolve over minutes without repeating the same
formula every frame.

### Phase 5: MilkDrop-inspired preset schema

Design a native JSON schema that can represent MilkDrop-like concepts without
being chained to `.milk` syntax:

- feedback
- warp
- waveform layers
- shape layers
- shader passes
- audio mapping
- director hints

Expected result: PAPAGEDON becomes a creator/runtime platform, not just a shader
pack runner.

## Recommendation

The first code task should be Phase 1: feedback ping-pong rendering. It is the
highest-impact improvement and aligns perfectly with PAPAGEDON's existing
OpenGL/FBO transition code. We already have enough rendering infrastructure to
add it without redesigning the engine.

After that, implement waveform/spectrum layers. That is the second most visible
gap versus projectM/MilkDrop.
