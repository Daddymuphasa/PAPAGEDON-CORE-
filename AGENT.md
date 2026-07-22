# PAPAGEDON Engineering Constitution

Version: 0.0.1

---

# Mission

PAPAGEDON is NOT a music visualizer.

PAPAGEDON is the infrastructure layer for immersive music experiences.

The long-term vision is to become the Unreal Engine for music-driven experiences, allowing developers to build reactive, adaptive, AI-generated audiovisual worlds across Desktop, Web, Mobile, XR, Live Events, Vehicles, Robotics, LED Walls, Projection Mapping, and future computing platforms.

Every engineering decision should support this vision.

---

# Core Philosophy

Never optimize for demos.

Always optimize for the future engine.

Every system should be:

- Modular
- Independent
- Replaceable
- Testable
- Platform agnostic
- Real-time safe

Avoid shortcuts that introduce technical debt.

---

# Development Principles

Priority Order

1. Correctness
2. Stability
3. Maintainability
4. Performance
5. Convenience

Never sacrifice architecture for speed.

---

# Architecture

Current roadmap

Stage 0
Vision & Architecture

Stage 1
Core Runtime

Stage 2
Audio Intelligence

Stage 3
Scene DNA

Stage 4
Experience Graph

Stage 5
Rendering Engine

Stage 6
Shader Runtime

Stage 7
Asset System

Stage 8
Plugin System

Stage 9
AI Experience Director

Stage 10
Creator Studio

Stage 11
Networking

Stage 12
Web Runtime

Stage 13
SDK

Stage 14
Optimization

Stage 15
Marketplace

Do not skip stages.

---

# Coding Standards

Use modern C++20.

Avoid macros unless unavoidable.

Prefer:

- RAII
- std::unique_ptr
- std::shared_ptr only when ownership is shared
- constexpr
- enum class
- std::span
- std::optional

Avoid global variables.

Avoid singleton abuse.

Keep functions small.

Keep classes focused.

No God Objects.

---

# Engine Philosophy

Everything is a system.

Examples:

Audio System

Scene System

Asset System

Rendering System

Input System

Networking System

Memory System

Plugin System

Each system should expose:

Initialize()

Update()

Shutdown()

No hidden dependencies.

---

# Runtime

The Runtime owns the engine.

The Runtime controls:

Initialization

Main Loop

Frame Timing

Update Order

Shutdown

Nothing bypasses the Runtime.

---

# Audio Intelligence

The Audio Intelligence layer is NOT the renderer.

Its only responsibility is converting sound into semantic information.

Example:

Audio

↓

Tempo

↓

Energy

↓

Emotion

↓

Tension

↓

Drops

↓

Experience Signals

The renderer consumes these signals.

---

# Experience Graph

The Experience Graph decides:

what should happen

NOT

how it is rendered.

Rendering is independent.

---

# Rendering

Rendering is replaceable.

Future renderers may include:

OpenGL

Vulkan

Metal

DirectX12

WebGPU

Software Renderer

Never tightly couple engine logic to rendering APIs.

---

# AI

AI does NOT directly draw graphics.

AI generates experience decisions.

Renderer executes.

---

# Plugins

Everything optional should become a plugin.

Never hardcode optional features.

---

# Performance Goals

Target:

120 FPS desktop

60 FPS web

60 FPS mobile

Low latency

Low memory allocation

Minimal frame spikes

Avoid allocations inside Update() whenever possible.

---

# Folder Structure

Future layout

/apps

/engine

/runtime

/core

/audio

/rendering

/scene

/experience

/ai

/sdk

/plugins

/platform

/editor

/tools

/tests

---

# Before Writing Code

Always ask:

Does this belong here?

Can this become a module?

Can this be reused?

Can this be replaced?

Will this scale?

---

# AI Assistant Rules

Do NOT redesign architecture without explicit approval.

Do NOT introduce unnecessary dependencies.

Do NOT rewrite working systems unless requested.

Prefer incremental changes.

Never remove documentation.

Never silently rename APIs.

Explain major refactors.

Always preserve engine modularity.

---

# Ultimate Goal

Build the world's best real-time immersive audio engine.

Every line of code should move the project toward that vision.