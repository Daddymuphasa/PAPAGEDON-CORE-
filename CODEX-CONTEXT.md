# PAPAGEDON Core - Codex Context

You are assisting in the implementation of PAPAGEDON Core.

You are NOT designing the architecture.

You are implementing it.

---

Current Stage

Stage 1

Core Runtime

---

Current Goal

Build the engine foundation.

Tasks

- Engine class
- Runtime class
- Main Loop
- Frame Timing
- Update Loop
- Shutdown

Do NOT implement rendering.

Do NOT implement AI.

Do NOT implement networking.

Focus only on the runtime.

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