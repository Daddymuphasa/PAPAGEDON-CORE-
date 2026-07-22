# PAPAGEDON Core - Codex Context

You are assisting in the implementation of PAPAGEDON Core.

You are NOT designing the architecture.

You are implementing it.

---

Current Stage

Stage 2

Audio Intelligence Foundation

---

Current Goal

Build the audio-intelligence foundation on top of the core runtime.

Tasks

- AudioFrame data contract
- ExperienceSignals data contract
- AudioAnalyzer interface
- Runtime audio-system update

Do NOT implement rendering.

Do NOT implement AI.

Do NOT implement networking.

Do NOT implement audio decoding, playback, or FFT analysis.

Focus on clean interfaces between the runtime and audio intelligence.

---

Current Repository

apps/
player/

Future folders

engine/

runtime/

core/

audio/

rendering/

scene/

experience/

ai/

sdk/

plugins/

---

Coding Style

Modern C++20

RAII

No globals

No macros

No singleton abuse

Small functions

Small classes

Readable code

Document public APIs.

---

Architecture Rules

main()

↓

Engine

↓

Runtime

↓

Systems

↓

Renderer

↓

Exit

Never place engine logic inside main.cpp.

main.cpp should remain minimal.

---

Allowed Refactors

Create folders

Move files

Rename symbols

Improve readability

Improve compilation

Improve architecture without changing behavior

---

Never Do

Do not redesign the engine.

Do not invent new architecture.

Do not add third-party libraries unless requested.

Do not introduce ECS unless instructed.

Do not tightly couple systems.

Do not skip roadmap stages.

---

Commit Style

Small commits.

One feature per commit.

Keep commits reversible.

---

Output Preference

When implementing:

1. Explain the change.

2. Show affected files.

3. Explain why.

4. Mention possible improvements.

Avoid giant rewrites.

---

Long-Term Vision

PAPAGEDON will become the runtime powering immersive music applications across:

Desktop

Web

Mobile

VR

AR

LED Walls

Projection Mapping

Vehicles

Robotics

Future computing platforms

Every implementation decision should support modularity, scalability, and real-time performance.
