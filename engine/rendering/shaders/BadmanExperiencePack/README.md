# Badman Experience Pack — Vol.1

Twelve premium cinematic fragment shaders for the **Badman Experience** rave brand —
dark-luxury underground built for **20 m LED walls, festival projection mapping,
nightclub visuals and large concert screens**.

Afro House · Amapiano · Tech House · luxury nightlife · red stage light · smoke ·
lasers · volumetric atmosphere.

## Palette
| role | hex | |
|------|-----|-|
| primary   | `#A30D18` | blood red |
| secondary | `#050505` | near black |
| accent    | `#D6A53A` | gold |
| highlight | `#F6F1EB` | cream |
| shadow    | `#180202` | deep shadow |

## The twelve
| # | Name | Energy | Recommended BPM |
|---|------|--------|-----------------|
| 01 | Hellsmoke        | medium | 118–126 |
| 02 | Red Chapel       | medium | 120–124 |
| 03 | Inferno Lasers   | high   | 124–130 |
| 04 | Obsidian Tunnel  | high   | 122–128 |
| 05 | Blood Pulse      | medium | 116–124 |
| 06 | Ember Field      | low    | 110–120 |
| 07 | Liquid Crimson   | medium | 112–122 |
| 08 | Black Sun        | high   | 122–128 |
| 09 | Rave Grid        | high   | 124–130 |
| 10 | Void Temple      | low    | 110–120 |
| 11 | Phantom Crowd    | medium | 118–126 |
| 12 | Final Ascension  | peak   | 126–132 |

## Layout
```
BadmanExperiencePack/
  manifest.json            pack manifest (all 12 + art direction + uniform contract)
  badman-theme.json        brand theme for the PAPAGEDON Theme System
  README.md
  shaders/   NN_name.frag  12 GLSL 460 fragment shaders
  metadata/  NN_name.json  12 metadata files (name, desc, mood, BPM, theme, energy)
```

## Integration (no engine changes)
Each shader is a complete `#version 460 core` fragment shader written against the
engine's existing contract:

- Vertex stage: the engine fullscreen-triangle (`gl_VertexID → vUV`).
- Uniforms: `uTime, uResolution, uBass, uMid, uTreble, uBeat, uEnergy, uIntensity`
  and theme colours `uPrimaryColour, uSecondaryColour, uAccentColour, uBackground`
  — the same names `ShaderManager` already sets. Unused uniforms resolve to −1 and
  are skipped. Theme colours fall back to the brand palette when unbound, so every
  shader keeps the Badman identity even standalone.

Load a `.frag` source and hand it to `ShaderManager::Compile(vertex, fragment)`;
select the `badman` theme in the Theme System for the branded palette. All twelve
compile + link against the live pipeline unchanged (see the pack manifest).

## Craft
- Frame-rate independent — all motion is driven by `uTime`.
- Filmic (ACES) tonemap + vignette on every shader for a graded, cinematic look.
- Shared 5-octave value-noise / fbm toolkit; bounded loops; no dynamic branching
  on data. Comfortable real-time at LED-wall resolution on integrated GPUs.
- No rainbows, no generic visualizer clichés — one coherent luxury-rave language.
