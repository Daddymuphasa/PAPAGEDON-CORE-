#include "OpenGLRenderer.h"
#include "../scene/SceneState.h"
#include "../ShaderUniforms.h"
#include "DebugOverlayRenderer.h"

#include <Theme.h>

#include <glad/glad.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace papagedon {

// ──────────────────────────────────────────────────────────────────────────────
// Preset-colour helpers
//
// The active ExperiencePreset supplies every colour the shader uses.  These free
// functions adapt the experience layer's PresetColor into the renderer's Color3
// and ease one colour toward another so preset switches cross-fade on screen.
// No colours are hardcoded here — they all originate from the preset table.
// ──────────────────────────────────────────────────────────────────────────────
namespace {

Color3 ToColor3(const visual::ThemeColor& c) noexcept {
    return Color3{c.r, c.g, c.b};
}

// ──────────────────────────────────────────────────────────────────────────────
// Dramatic shader transition — 10 randomised effects driven by render-to-texture
// crossfade.  FBO-A holds a snapshot of the outgoing shader; FBO-B holds the
// live incoming shader.  The transition shader blends between the two textures
// with the selected effect.  Every switch picks a random effect so no two
// transitions at the booth look the same.
// ──────────────────────────────────────────────────────────────────────────────
constexpr int kNumTransitionEffects = 10;

constexpr float kTransitionDurations[kNumTransitionEffects] = {
    0.35f,  // 0  FLASH_BANG
    0.40f,  // 1  GLITCH_TEAR
    0.40f,  // 2  RADIAL_WIPE
    0.35f,  // 3  ZOOM_BLAST
    0.35f,  // 4  STROBE_CUT
    0.35f,  // 5  DIAGONAL_SLASH
    0.45f,  // 6  SPIRAL_DISSOLVE
    0.40f,  // 7  SHATTER
    0.40f,  // 8  RGB_SPLIT
    0.40f,  // 9  MELT
};

constexpr const char* kTransitionFragment = R"GLSL(
#version 330 core
in  vec2 vUV;
out vec4 fragColor;

uniform sampler2D uTexA;       // outgoing shader (snapshot)
uniform sampler2D uTexB;       // incoming shader (live)
uniform float     uProgress;   // 0 → 1
uniform int       uEffect;     // which dramatic effect
uniform float     uTime;       // global clock for animation
uniform vec2      uResolution; // framebuffer size

// ── helpers ──────────────────────────────────────────────────────────────────
float hash(vec2 p) {
    p = fract(p * vec2(123.34, 345.45));
    p += dot(p, p + 34.345);
    return fract(p.x * p.y);
}
float hash1(float n) { return fract(sin(n) * 43758.5453); }

void main() {
    vec2 uv = vUV;
    float p = uProgress;
    vec4 a  = texture(uTexA, uv);
    vec4 b  = texture(uTexB, uv);
    vec4 result;

    // 0 ── FLASH BANG ─────────────────────────────────────────────────────────
    // Violent white-out flash then slam to the new shader.
    if (uEffect == 0) {
        float flash = exp(-p * 9.0) * 2.5;
        float blend = smoothstep(0.12, 0.45, p);
        result = mix(a, b, blend);
        result.rgb += vec3(flash);
        result.rgb *= 1.0 + (1.0 - p) * 0.4;
    }

    // 1 ── GLITCH TEAR ────────────────────────────────────────────────────────
    // Horizontal block displacement, RGB split, white scanline bursts.
    else if (uEffect == 1) {
        float glitchAmt = sin(p * 3.14159) * 0.9;
        float blockY    = floor(uv.y * 24.0);
        float blockHash = hash(vec2(blockY, floor(uTime * 25.0)));
        float shift     = (blockHash - 0.5) * glitchAmt * 0.25;
        vec2  uvG       = uv + vec2(shift, 0.0);
        float blend     = smoothstep(0.25, 0.75,
                              p + (hash(vec2(blockY, 1.0)) - 0.5) * 0.35);
        float rgbOff    = glitchAmt * 0.025;
        result.r = mix(texture(uTexA, uvG + vec2( rgbOff, 0.0)).r,
                       texture(uTexB, uvG + vec2( rgbOff, 0.0)).r, blend);
        result.g = mix(texture(uTexA, uvG).g,
                       texture(uTexB, uvG).g, blend);
        result.b = mix(texture(uTexA, uvG + vec2(-rgbOff, 0.0)).b,
                       texture(uTexB, uvG + vec2(-rgbOff, 0.0)).b, blend);
        result.a = 1.0;
        float scanline = step(0.96, hash(vec2(blockY * 3.0,
                              floor(uTime * 50.0)))) * glitchAmt;
        result.rgb += vec3(scanline);
    }

    // 2 ── RADIAL WIPE ────────────────────────────────────────────────────────
    // Energy ring expanding from the centre reveals the new shader.
    else if (uEffect == 2) {
        vec2  c    = (uv - 0.5) * vec2(uResolution.x / uResolution.y, 1.0);
        float dist = length(c);
        float rad  = p * 1.6;
        float edge = smoothstep(rad - 0.06, rad + 0.06, dist);
        result = mix(b, a, edge);
        float ring = exp(-pow((dist - rad) / 0.025, 2.0));
        result.rgb += vec3(ring) * 1.2;
    }

    // 3 ── ZOOM BLAST ─────────────────────────────────────────────────────────
    // Old shader zooms to infinity; new one blasts outward from a point.
    else if (uEffect == 3) {
        vec2 zoomA = (uv - 0.5) / (1.0 + p * 5.0) + 0.5;
        vec2 zoomB = (uv - 0.5) / max(0.05, 1.0 - (1.0 - p) * 3.0) + 0.5;
        zoomB = clamp(zoomB, 0.0, 1.0);
        float blend = smoothstep(0.25, 0.50, p);
        vec4 zA = texture(uTexA, clamp(zoomA, 0.0, 1.0));
        vec4 zB = texture(uTexB, zoomB);
        result = mix(zA, zB, blend);
        float flash = exp(-pow((p - 0.35) / 0.07, 2.0)) * 2.0;
        result.rgb += vec3(flash);
    }

    // 4 ── STROBE CUT ─────────────────────────────────────────────────────────
    // Alternates old/new at increasing speed, then hard-cuts.
    else if (uEffect == 4) {
        float freq   = 4.0 + p * 28.0;
        float strobe = step(0.5, fract(p * freq));
        float cut    = step(0.72, p);
        result = mix(strobe > 0.5 ? b : a, b, cut);
        float edgeF = fract(p * freq);
        float fl    = exp(-pow(min(edgeF, 1.0 - edgeF) / 0.04, 2.0)) * 0.35;
        result.rgb += vec3(fl);
    }

    // 5 ── DIAGONAL SLASH ─────────────────────────────────────────────────────
    // Bright diagonal line slashes across the screen.
    else if (uEffect == 5) {
        float ang  = 0.72;
        float line = uv.x * cos(ang) + uv.y * sin(ang);
        float wipe = smoothstep(p * 1.8 - 0.42, p * 1.8 - 0.38, line);
        result = mix(b, a, wipe);
        float edgeDist = abs(line - (p * 1.8 - 0.40));
        result.rgb += vec3(exp(-edgeDist * 90.0) * 1.8);
    }

    // 6 ── SPIRAL DISSOLVE ────────────────────────────────────────────────────
    // Spiral pattern that eats away the old image.
    else if (uEffect == 6) {
        vec2  c      = (uv - 0.5) * vec2(uResolution.x / uResolution.y, 1.0);
        float angle  = atan(c.y, c.x);
        float dist   = length(c);
        float spiral = fract(angle / 6.28318 + dist * 3.5 - p * 2.5);
        float diss   = smoothstep(p - 0.12, p + 0.12,
                            spiral * (1.0 - dist * 0.25));
        result = mix(a, b, diss);
        float sparkle = exp(-pow((spiral - p) / 0.02, 2.0)) * 0.6;
        result.rgb += vec3(sparkle);
    }

    // 7 ── SHATTER ────────────────────────────────────────────────────────────
    // Screen breaks into cells that flip to reveal the new shader.
    else if (uEffect == 7) {
        float scale   = 10.0;
        vec2  cell    = floor(uv * scale);
        vec2  local   = fract(uv * scale);
        float cHash   = hash(cell);
        float flipT   = clamp((p - cHash * 0.45) * 2.8, 0.0, 1.0);
        float flip    = smoothstep(0.0, 1.0, flipT);
        vec2  disp    = (vec2(hash(cell + 1.0), hash(cell + 2.0)) - 0.5)
                        * 0.04 * sin(flipT * 3.14159);
        vec2  uvD     = uv + disp;
        result = mix(texture(uTexA, uvD), texture(uTexB, uvD), flip);
        float eX   = min(local.x, 1.0 - local.x);
        float eY   = min(local.y, 1.0 - local.y);
        float cEdge = 1.0 - smoothstep(0.0, 0.06, min(eX, eY));
        result.rgb += vec3(cEdge * sin(flipT * 3.14159) * 0.6);
    }

    // 8 ── RGB SPLIT ──────────────────────────────────────────────────────────
    // Chromatic aberration tears the image apart, reassembles as new.
    else if (uEffect == 8) {
        float splitAmt = sin(p * 3.14159) * 0.09;
        float blend    = smoothstep(0.30, 0.70, p);
        vec2 d1 = vec2(cos(uTime * 2.0),          sin(uTime * 2.0))          * splitAmt;
        vec2 d2 = vec2(cos(uTime * 2.0 + 2.094),  sin(uTime * 2.0 + 2.094)) * splitAmt;
        vec2 d3 = vec2(cos(uTime * 2.0 + 4.189),  sin(uTime * 2.0 + 4.189)) * splitAmt;
        result.r = mix(texture(uTexA, uv + d1).r, texture(uTexB, uv + d1).r, blend);
        result.g = mix(texture(uTexA, uv + d2).g, texture(uTexB, uv + d2).g, blend);
        result.b = mix(texture(uTexA, uv + d3).b, texture(uTexB, uv + d3).b, blend);
        result.a = 1.0;
    }

    // 9 ── MELT ───────────────────────────────────────────────────────────────
    // Old shader melts downward like hot wax.
    else if (uEffect == 9) {
        float wave     = sin(uv.x * 16.0) * 0.08 + sin(uv.x * 8.0 + 1.5) * 0.12;
        float meltLine = p * 1.5 - 0.25 + wave;
        float drip     = smoothstep(meltLine - 0.08, meltLine, uv.y);
        float meltZone = 1.0 - smoothstep(meltLine - 0.12, meltLine, uv.y);
        vec2  meltUV   = uv;
        meltUV.y += meltZone * 0.06 * sin(uv.x * 22.0 + uTime * 6.0);
        result = mix(texture(uTexB, meltUV), texture(uTexA, meltUV), drip);
        float dripEdge = exp(-pow((uv.y - meltLine) / 0.012, 2.0));
        result.rgb += vec3(dripEdge * 0.7);
    }

    // fallback
    else {
        result = mix(a, b, p);
    }

    fragColor = result;
}
)GLSL";

// ──────────────────────────────────────────────────────────────────────────────
constexpr const char* kDeformationTransitionFragment = R"GLSL(
#version 330 core
in vec2 vUV;
out vec4 fragColor;
uniform sampler2D uTexA;
uniform sampler2D uTexB;
uniform float uProgress;
uniform float uTime;
uniform vec2 uResolution;
void main() {
    float p = smoothstep(0.0, 1.0, uProgress);
    vec2 aspect = vec2(uResolution.x / uResolution.y, 1.0);
    vec2 q = (vUV - 0.5) * aspect;
    float radius = length(q);
    float angle = atan(q.y, q.x);
    float envelope = sin(3.14159265 * p);
    float twist = envelope * (0.38 + radius * 0.62);
    float c = cos(twist), s = sin(twist);
    vec2 turned = mat2(c, -s, s, c) * q;
    turned *= 1.0 + envelope * (0.10 * sin(angle * 3.0 + uTime * 0.7) - 0.08);
    vec2 bend = turned - q;
    vec2 uvA = clamp(0.5 + (q - bend * (1.0 - p)) / aspect, 0.002, 0.998);
    vec2 uvB = clamp(0.5 + (q + bend * p) / aspect, 0.002, 0.998);
    float field = vUV.x + 0.075 * sin(vUV.y * 8.0 + angle * 2.0);
    float reveal = smoothstep(p - 0.16, p + 0.16, field);
    float seam = exp(-abs(field - p) * 34.0) * envelope;
    fragColor = mix(texture(uTexB, uvB), texture(uTexA, uvA), reveal);
    fragColor.rgb += vec3(0.23, 0.18, 0.36) * seam;
}
)GLSL";

// PGX feedback composite pass
//
// The current visual is rendered into uScene.  The previous composited frame is
// sampled from uPrevious, warped/decayed according to native PGX controls, then
// blended with the live scene.  This gives PAPAGEDON the MilkDrop/projectM-style
// visual memory loop without adopting the .milk runtime model.
// ──────────────────────────────────────────────────────────────────────────────
constexpr const char* kPgxFeedbackFragment = R"GLSL(
#version 330 core
in  vec2 vUV;
out vec4 fragColor;

uniform sampler2D uScene;
uniform sampler2D uPrevious;
uniform float uTime;
uniform vec2  uResolution;
uniform float uDecay;
uniform float uZoom;
uniform float uRotation;
uniform float uWarp;
uniform float uBeatWarp;
uniform float uBeat;
uniform float uEnergy;

void main() {
    vec2 uv = vUV;
    vec2 p = uv - 0.5;
    p.x *= uResolution.x / max(uResolution.y, 1.0);

    float decay = clamp(uDecay, 0.0, 0.985);
    float warp = uWarp + uBeatWarp * uBeat;

    float ca = cos(uRotation * (0.45 + uEnergy));
    float sa = sin(uRotation * (0.45 + uEnergy));
    p = mat2(ca, -sa, sa, ca) * p;

    float zoom = mix(1.0, max(uZoom, 0.001), decay);
    p /= zoom;

    vec2 n = vec2(
        sin((p.y * 8.0) + uTime * 0.70 + uEnergy * 2.0),
        cos((p.x * 7.0) - uTime * 0.65 + uBeat * 4.0)
    );
    p += n * warp * 0.045 * decay;

    p.x /= uResolution.x / max(uResolution.y, 1.0);
    vec2 prevUV = p + 0.5;

    vec4 scene = texture(uScene, uv);
    vec4 prev = vec4(0.0);
    if (all(greaterThanEqual(prevUV, vec2(0.0))) &&
        all(lessThanEqual(prevUV, vec2(1.0)))) {
        prev = texture(uPrevious, prevUV) * decay;
    }

    float liveMix = clamp(0.68 + uEnergy * 0.18 + uBeat * 0.12, 0.62, 0.92);
    vec3 color = max(scene.rgb, prev.rgb * (0.82 + decay * 0.18));
    color = mix(prev.rgb, color, liveMix);
    color += scene.rgb * scene.rgb * (0.05 + uBeat * 0.10);

    fragColor = vec4(clamp(color, 0.0, 1.0), 1.0);
}
)GLSL";

// ──────────────────────────────────────────────────────────────────────────────
// PGX audio geometry / shape pass
//
// Samples compact GPU audio textures generated by AudioAnalyzer: 64 spectrum
// bands and 128 waveform samples. The pass is additive and renderer-owned, so
// visible music geometry is no longer faked inside individual fragment shaders.
// ──────────────────────────────────────────────────────────────────────────────
constexpr const char* kPgxAudioShapeFragment = R"GLSL(
#version 330 core
in  vec2 vUV;
out vec4 fragColor;

uniform sampler2D uSpectrum;
uniform sampler2D uWaveform;
uniform float uTime;
uniform vec2  uResolution;
uniform int   uWaveformMode;
uniform float uWaveformOpacity;
uniform float uWaveformThickness;
uniform float uWaveformRadius;
uniform float uWaveformBassResponse;
uniform float uWaveformTrebleResponse;
uniform float uBass;
uniform float uMid;
uniform float uTreble;
uniform float uBeat;
uniform float uEnergy;
uniform float uPhase;
uniform float uSeed;
uniform vec3  uPrimaryColour;
uniform vec3  uSecondaryColour;
uniform vec3  uAccentColour;

const float kPi = 3.14159265358979;
const float kTau = 6.28318530717959;

float hash(float n) { return fract(sin(n) * 43758.5453123); }
float spec(float x) { return texture(uSpectrum, vec2(clamp(x, 0.0, 1.0), 0.5)).r; }
float wave(float x) { return texture(uWaveform, vec2(clamp(x, 0.0, 1.0), 0.5)).r; }

float lineSegment(vec2 p, vec2 a, vec2 b, float width) {
    vec2 pa = p - a;
    vec2 ba = b - a;
    float h = clamp(dot(pa, ba) / max(dot(ba, ba), 1.0e-5), 0.0, 1.0);
    return smoothstep(width, 0.0, length(pa - ba * h));
}

void main() {
    vec2 uv = (vUV * 2.0 - 1.0) * vec2(uResolution.x / max(uResolution.y, 1.0), 1.0);
    vec2 uv01 = vUV;
    vec3 color = vec3(0.0);

    float audioPush = uBass * uWaveformBassResponse + uTreble * uWaveformTrebleResponse;
    float opacity = uWaveformOpacity * (0.55 + uEnergy * 0.85 + uBeat * 0.8);
    float width = 0.006 * max(uWaveformThickness, 0.2);

    // Real waveform layer.
    if (uWaveformMode == 1) {
        float sample = wave(uv01.x);
        float y = sample * (0.13 + audioPush * 0.18);
        float glow = smoothstep(width * 5.0, 0.0, abs(uv.y - y));
        float core = smoothstep(width, 0.0, abs(uv.y - y));
        color += uAccentColour * glow * opacity * 0.55;
        color += mix(uPrimaryColour, vec3(1.0), 0.25) * core * opacity * 1.6;
    } else if (uWaveformMode == 2 || uWaveformMode == 3) {
        float a = atan(uv.y, uv.x) + kPi;
        float x = fract(a / kTau + uPhase * 0.015);
        float sample = wave(x);
        float r = length(uv);
        float target = uWaveformRadius + sample * (0.07 + audioPush * 0.12);
        float band = smoothstep(width * 5.5, 0.0, abs(r - target));
        float core = smoothstep(width * 1.4, 0.0, abs(r - target));
        float ribbon = uWaveformMode == 2 ? 0.55 + 0.45 * sin(a * 9.0 + uPhase) : 1.0;
        color += mix(uSecondaryColour, uAccentColour, spec(x)) * band * opacity * ribbon;
        color += vec3(1.0) * core * opacity * 0.8;
    }

    // Radial 64-band spectrum bars.
    float r = length(uv);
    float a = atan(uv.y, uv.x) + kPi;
    float slot = a / kTau * 64.0;
    float bandId = floor(slot);
    float fracSlot = fract(slot);
    float band = spec((bandId + 0.5) / 64.0);
    float inner = 0.22 + uBass * 0.04;
    float outer = inner + 0.15 + band * (0.55 + uEnergy * 0.20);
    float angular = smoothstep(0.46, 0.24, abs(fracSlot - 0.5));
    float bar = angular * smoothstep(inner, inner + 0.015, r) * smoothstep(outer + 0.025, outer, r);
    color += mix(uPrimaryColour, uAccentColour, band) * bar * (0.30 + uEnergy * 0.45);

    // Beat rings.
    for (int i = 0; i < 4; ++i) {
        float age = fract(uPhase * 0.11 + float(i) * 0.25);
        float rr = age * (1.25 + uBass * 0.25);
        float ring = smoothstep(0.020, 0.0, abs(r - rr)) * (1.0 - age);
        color += uAccentColour * ring * (0.20 + uBeat * 1.4);
    }

    // Line web traces, driven by mid/treble energy.
    for (int i = 0; i < 10; ++i) {
        float fi = float(i);
        float h1 = hash(fi * 17.0 + floor(uSeed));
        float h2 = hash(fi * 31.0 + 9.0 + floor(uSeed));
        float t = uPhase * (0.10 + h1 * 0.05) + fi;
        vec2 p1 = vec2(sin(t + h1 * kTau), cos(t * 0.81 + h2 * kTau)) * (0.18 + h1 * 0.58);
        vec2 p2 = vec2(cos(t * 0.73 + h2 * kTau), sin(t * 1.13 + h1 * kTau)) * (0.24 + h2 * 0.55);
        float trace = lineSegment(uv, p1, p2, 0.004 + uTreble * 0.009);
        color += mix(uSecondaryColour, uAccentColour, h1) * trace * (0.05 + uMid * 0.22 + uTreble * 0.25);
    }

    // Spark bursts on high frequencies.
    for (int i = 0; i < 24; ++i) {
        float fi = float(i);
        float h = hash(fi * 23.1 + floor(uSeed));
        float ang = h * kTau + uPhase * (0.15 + hash(fi) * 0.12);
        float dist = 0.18 + hash(fi + 4.0) * 1.05;
        vec2 sp = vec2(cos(ang), sin(ang)) * dist;
        float sz = 0.006 + spec(h) * 0.020;
        float spark = exp(-dot(uv - sp, uv - sp) / max(sz * sz, 1.0e-5));
        color += uAccentColour * spark * (uTreble * 0.18 + uBeat * 0.22);
    }

    // Tunnel overlays: depth bands that accelerate with energy.
    float tunnel = 0.5 + 0.5 * sin(1.0 / max(r, 0.05) * 4.0 - uPhase * (1.5 + uEnergy * 2.0) + a * 4.0);
    tunnel = smoothstep(0.78, 1.0, tunnel) * smoothstep(1.25, 0.05, r);
    color += uPrimaryColour * tunnel * (0.04 + uBass * 0.16 + uBeat * 0.18);

    fragColor = vec4(clamp(color, 0.0, 1.0), clamp(length(color), 0.0, 1.0));
}
)GLSL";

// ──────────────────────────────────────────────────────────────────────────────
// Cinematic startup logo — procedural gold "waveform → P" mark on deep green.
// A luxury brand reveal: the audio waveform draws in from the left, flows into
// the P (which draws bottom-to-top), a gold gleam sweeps across, and it glows,
// then fades into the show.  uTime is seconds since the intro began.
// ──────────────────────────────────────────────────────────────────────────────
constexpr const char* kIntroFragment = R"GLSL(
#version 330 core
in  vec2 vUV;
out vec4 fragColor;
uniform float uTime;
uniform vec2  uResolution;

float sdSeg(vec2 p, vec2 a, vec2 b){
    vec2 pa = p - a, ba = b - a;
    float h = clamp(dot(pa, ba) / dot(ba, ba), 0.0, 1.0);
    return length(pa - ba * h);
}
float hash(vec2 p){ p = fract(p * vec2(123.34, 345.45)); p += dot(p, p + 34.345); return fract(p.x * p.y); }

void main(){
    vec2 R = uResolution;
    vec2 uv = vUV * 2.0 - 1.0;
    uv.x *= R.x / R.y;
    float T = uTime;

    // ── deep forest-green backdrop, soft centre light + vignette + grain ──
    float vig = smoothstep(1.7, 0.15, length(uv));
    vec3 col  = mix(vec3(0.018, 0.050, 0.032), vec3(0.055, 0.115, 0.082), vig);
    col += (hash(vUV * R) - 0.5) * 0.015;

    // logo space (slight settle from large → resting)
    vec2 p = uv * 1.35;
    p /= mix(1.07, 1.0, smoothstep(0.0, 1.3, T));

    float sx = -0.16;                 // stem x
    float th = 0.028;                 // stroke half-thickness
    float aa = 2.4 * 1.35 / R.y;      // ~2px anti-alias in this space

    // ── P mark: vertical stem + right-half bowl ──
    float dStem = sdSeg(p, vec2(sx, -0.62), vec2(sx, 0.60));
    vec2  c  = vec2(sx, 0.24);
    float Rb = 0.36;
    float dBowl = (p.x < sx) ? 1e3 : abs(length(p - c) - Rb);
    float dP = min(dStem, dBowl);

    // ── waveform: audio burst entering from the left into the stem ──
    float xw = p.x;
    float wob = 0.30 * sin(xw * 10.0) * exp(-pow((xw + 0.55) / 0.28, 2.0))
              + 0.44 * sin(xw * 16.0) * exp(-pow((xw + 0.30) / 0.20, 2.0))
              + 0.22 * sin(xw * 22.0) * exp(-pow((xw + 0.05) / 0.16, 2.0));
    float baseline = -0.03 + wob;
    float inSpan = step(-1.30, p.x) * step(p.x, sx + 0.01);
    float dWave = mix(1e3, abs(p.y - baseline), inSpan);

    // ── time-driven draw-on reveals ──
    float waveProg  = smoothstep(0.35, 1.9, T);
    float waveFront = mix(-1.34, sx + 0.02, waveProg);
    float waveMask  = 1.0 - smoothstep(waveFront - 0.05, waveFront, p.x);

    float pProg   = smoothstep(1.5, 3.0, T);
    float pFrontY = mix(-0.72, 0.66, pProg);
    float pMask   = 1.0 - smoothstep(pFrontY - 0.06, pFrontY, p.y);

    float waveLine = (1.0 - smoothstep(th - aa, th + aa, dWave)) * waveMask;
    float pLine    = (1.0 - smoothstep(th - aa, th + aa, dP))    * pMask;
    float mark     = max(waveLine, pLine);
    float dMark    = min(dWave, dP);

    // ── gold shading (vertical gradient) ──
    float grad = clamp(p.y * 0.55 + 0.5, 0.0, 1.0);
    vec3 gold  = mix(vec3(0.45, 0.30, 0.07), vec3(1.0, 0.87, 0.53), grad);

    // gleam sweep across the mark
    float sweepEnv = smoothstep(2.2, 3.4, T) - smoothstep(3.4, 4.3, T);
    float proj     = dot(p, normalize(vec2(0.9, 0.5)));
    float sweepPos = mix(-0.9, 0.95, smoothstep(2.2, 3.7, T));
    float gleam    = exp(-pow((proj - sweepPos) / 0.10, 2.0)) * sweepEnv;
    gold = mix(gold, vec3(1.0, 0.97, 0.86), gleam * 0.9);

    // ── glow halo + bright leading "pen" tips ──
    float glow = exp(-dMark * 10.0) * max(waveMask, pMask);
    float waveTip = exp(-pow((p.x - waveFront) / 0.02, 2.0))
                  * exp(-pow((p.y - baseline) / 0.05, 2.0)) * (1.0 - step(1.85, T));
    float pTip = exp(-pow((p.y - pFrontY) / 0.02, 2.0))
               * (1.0 - smoothstep(th * 3.0, th * 6.0, dP))
               * step(1.5, T) * (1.0 - step(3.0, T));

    // ── compose ──
    col += vec3(0.9, 0.65, 0.25) * glow * 0.45;
    col  = mix(col, gold, clamp(mark, 0.0, 1.0));
    col += vec3(1.0, 0.95, 0.8) * (waveTip + pTip) * 0.85;
    col += gold * gleam * 0.5;

    // ── global fades ──
    float inFade  = smoothstep(0.0, 0.5, T);
    float outFade = 1.0 - smoothstep(4.35, 4.9, T);
    col *= inFade * outFade;
    col *= mix(0.62, 1.0, vig);

    fragColor = vec4(col, 1.0);
}
)GLSL";

// Compiles a vertex + fragment pair into a linked program (0 on failure).
[[nodiscard]] unsigned int CompileGLProgram(const char* vs, const char* fs) noexcept {
    const auto stage = [](GLenum type, const char* src) -> unsigned int {
        const unsigned int sh = glCreateShader(type);
        glShaderSource(sh, 1, &src, nullptr);
        glCompileShader(sh);
        int ok = 0;
        glGetShaderiv(sh, GL_COMPILE_STATUS, &ok);
        if (ok == 0) {
            char log[512];
            glGetShaderInfoLog(sh, 512, nullptr, log);
            std::fprintf(stderr, "[fade] shader error: %s\n", log);
            glDeleteShader(sh);
            return 0u;
        }
        return sh;
    };
    const unsigned int v = stage(GL_VERTEX_SHADER, vs);
    if (v == 0u) return 0u;
    const unsigned int f = stage(GL_FRAGMENT_SHADER, fs);
    if (f == 0u) { glDeleteShader(v); return 0u; }
    const unsigned int p = glCreateProgram();
    glAttachShader(p, v);
    glAttachShader(p, f);
    glLinkProgram(p);
    glDeleteShader(v);
    glDeleteShader(f);
    int ok = 0;
    glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if (ok == 0) { glDeleteProgram(p); return 0u; }
    return p;
}

// Extracts a JSON integer value for a given key from a flat JSON string.
// Returns `fallback` if the key is not found.
int JsonInt(const std::string& json, const char* key, int fallback) noexcept {
    const std::string needle = std::string("\"") + key + "\"";
    const auto pos = json.find(needle);
    if (pos == std::string::npos) return fallback;
    auto colon = json.find(':', pos + needle.size());
    if (colon == std::string::npos) return fallback;
    ++colon;
    while (colon < json.size() && (json[colon] == ' ' || json[colon] == '\t')) ++colon;
    return std::atoi(json.c_str() + colon);
}

// Extracts a JSON string value for a given key.
std::string JsonString(const std::string& json, const char* key) noexcept {
    const std::string needle = std::string("\"") + key + "\"";
    const auto pos = json.find(needle);
    if (pos == std::string::npos) return {};
    auto q1 = json.find('"', json.find(':', pos + needle.size()) + 1);
    if (q1 == std::string::npos) return {};
    auto q2 = json.find('"', q1 + 1);
    if (q2 == std::string::npos) return {};
    return json.substr(q1 + 1, q2 - q1 - 1);
}

// Checks whether a JSON array-of-strings contains a given value.
bool JsonArrayContains(const std::string& json, const char* arrayKey,
                       const char* value) noexcept {
    const std::string needle = std::string("\"") + arrayKey + "\"";
    const auto pos = json.find(needle);
    if (pos == std::string::npos) return false;
    auto bracket = json.find('[', pos + needle.size());
    if (bracket == std::string::npos) return false;
    auto end = json.find(']', bracket);
    if (end == std::string::npos) return false;
    return json.substr(bracket, end - bracket).find(value) != std::string::npos;
}

Color3 LerpColor(const Color3& a, const Color3& b, const float t) noexcept {
    return Color3{
        a.r + (b.r - a.r) * t,
        a.g + (b.g - a.g) * t,
        a.b + (b.b - a.b) * t,
    };
}

// One-pole follower with asymmetric attack/release, frame-rate independent.
// A fast attack keeps visual onsets in sync with the audio (low latency); a
// slower release smooths the decay so nothing strobes between frames.
float Follow(const float current, const float target, const float dt,
             const float attackRate, const float releaseRate) noexcept {
    const float rate  = target > current ? attackRate : releaseRate;
    const float alpha = 1.0f - std::exp(-dt * rate);
    return current + (target - current) * alpha;
}

} // namespace

// ──────────────────────────────────────────────────────────────────────────────
// Implementation (pimpl)
// ──────────────────────────────────────────────────────────────────────────────
class OpenGLRenderer::Implementation final {
public:
    GLFWwindow* window              = nullptr;
    double      lastFpsUpdateTime   = 0.0;
    unsigned int renderedFrameCount = 0;
    double      currentFps          = 0.0;
    double      lastFrameTime       = 0.0;
    bool        showDebugOverlay    = false;
    bool        debugKeyWasPressed  = false;
    DebugOverlayRenderer debugOverlay;
    ShaderUniforms smoothedUniforms;

    // ── Shader library ──────────────────────────────────────────────────────────
    // Slot 0 is the built-in reactive shader (its own 12 forms via Auto-VJ); the
    // rest are premium pack shaders (e.g. Badman Experience) compiled from .frag
    // files at startup. '[' / ']' cycle the active shader live for a VJ set.
    std::vector<std::unique_ptr<ShaderManager>> shaderLib;   // extra shaders (slot 1..N)
    std::vector<std::string>                    shaderNames; // names for every slot (0..N)
    int  activeShader        = 0;
    bool prevShaderKeyPressed = false;
    bool nextShaderKeyPressed = false;
    // Brief on-screen name toast after a switch.
    std::string toastText;
    double      toastUntil = 0.0;

    // ── Dramatic shader transition (FBO crossfade) ────────────────────────────────
    // Two framebuffers capture outgoing (snapshot) and incoming (live) shader
    // frames; a transition shader blends between them with one of 10 dramatic
    // randomised effects.  No more plain fade-to-black.
    unsigned int transitionFBO[2]  = {0, 0};
    unsigned int transitionTex[2]  = {0, 0};
    int          transitionTexW    = 0;
    int          transitionTexH    = 0;
    unsigned int transitionProgram = 0;
    int          txLocTexA       = -1;
    int          txLocTexB       = -1;
    int          txLocProgress   = -1;
    int          txLocEffect     = -1;
    int          txLocTime       = -1;
    int          txLocResolution = -1;
    bool         transitioning      = false;
    int          pendingShader      = -1;
    int          outgoingShader     = 0;
    int          transitionEffect   = 0;
    int          lastTransitionEffect = -1;
    float        transitionProgress = 0.0f;
    bool         needsOutgoingCapture = false;

    // ── PGX visual-memory pipeline ─────────────────────────────────────────────
    unsigned int pgxSceneFBO = 0;
    unsigned int pgxSceneTex = 0;
    unsigned int pgxFeedbackFBO[2] = {0, 0};
    unsigned int pgxFeedbackTex[2] = {0, 0};
    int          pgxTexW = 0;
    int          pgxTexH = 0;
    int          pgxReadIndex = 0;
    bool         pgxFeedbackPrimed = false;
    unsigned int pgxFeedbackProgram = 0;
    int          pgxLocScene = -1;
    int          pgxLocPrevious = -1;
    int          pgxLocTime = -1;
    int          pgxLocResolution = -1;
    int          pgxLocDecay = -1;
    int          pgxLocZoom = -1;
    int          pgxLocRotation = -1;
    int          pgxLocWarp = -1;
    int          pgxLocBeatWarp = -1;
    int          pgxLocBeat = -1;
    int          pgxLocEnergy = -1;
    unsigned int pgxSpectrumTex = 0;
    unsigned int pgxWaveformTex = 0;
    unsigned int pgxAudioShapeProgram = 0;
    int          pgxShapeLocSpectrum = -1;
    int          pgxShapeLocWaveform = -1;
    int          pgxShapeLocTime = -1;
    int          pgxShapeLocResolution = -1;
    int          pgxShapeLocWaveformMode = -1;
    int          pgxShapeLocWaveformOpacity = -1;
    int          pgxShapeLocWaveformThickness = -1;
    int          pgxShapeLocWaveformRadius = -1;
    int          pgxShapeLocWaveformBassResponse = -1;
    int          pgxShapeLocWaveformTrebleResponse = -1;
    int          pgxShapeLocBass = -1;
    int          pgxShapeLocMid = -1;
    int          pgxShapeLocTreble = -1;
    int          pgxShapeLocBeat = -1;
    int          pgxShapeLocEnergy = -1;
    int          pgxShapeLocPhase = -1;
    int          pgxShapeLocSeed = -1;
    int          pgxShapeLocPrimary = -1;
    int          pgxShapeLocSecondary = -1;
    int          pgxShapeLocAccent = -1;
    PresetId     pgxStatePreset = PresetId::Aurora;
    bool         pgxStateInitialized = false;
    float        pgxPhase = 0.0f;
    float        pgxSeed = 1.0f;

    // ── Cinematic startup intro (procedural gold logo) ──────────────────────────
    unsigned int introProgram = 0;
    int          introTimeLoc = -1;
    int          introResLoc  = -1;
    bool         introActive  = true;   // plays once at startup
    double       introStart   = -1.0;

    // ── Live controls menu (H, and auto-shown at startup) ───────────────────────
    bool   showMenu          = false;
    bool   menuKeyWasPressed = false;   // 'H'
    double menuVisibleUntil  = 0.0;     // auto-hide time when shown non-sticky

    // ── Auto-shader director (music-driven transitions) ─────────────────────────
    bool         autoShader     = false;
    double       lastSwitchTime = 0.0;
    bool         prevBeat       = false;
    float        energyBaseline = 0.0f;
    bool         autoShaderKeyWasPressed = false; // 'V'

    // ── Pattern cross-fade state ───────────────────────────────────────────────
    // Tracks the signature form the shader is drawing.  When the preset's pattern
    // changes we snapshot the outgoing form and ease patternBlend 0 → 1, so the
    // visual form morphs across the switch rather than popping.
    int   activePatternMode   = 0;
    int   previousPatternMode = 0;
    float patternBlend        = 1.0f;
    bool  patternInitialized  = false;

    // ── Live controls (Task 7) ──────────────────────────────────────────────────
    // F1..F7 select a theme slot; Space toggles play/pause; R reloads the theme
    // JSON; A toggles Auto-VJ.  Rising edges are latched in EndFrame (where events
    // are polled) and drained by the Runtime via the Consume* methods.  F12
    // (overlay) and ESC (exit) are handled inside EndFrame directly.
    static constexpr int kThemeKeyCount = 7;
    bool themeKeyWasPressed[kThemeKeyCount] = {};
    int  pendingThemeRequest = -1;

    // 'B' selects TRANCE and toggles its wordmark; 'I' cycles live audio devices.
    bool tranceKeyWasPressed = false;
    bool pendingTranceRequest = false;
    bool showTranceWordmark = false;
    double tranceWordmarkStart = 0.0;
    bool badmanKeyWasPressed = false;
    bool pendingBadmanRequest = false;
    bool showBadmanWordmark = false;
    double badmanWordmarkStart = 0.0;

    bool inputKeyWasPressed = false;
    bool pendingInputSwitch = false;

    bool spaceWasPressed  = false;
    bool pendingPlayPause = false;

    bool reloadKeyWasPressed = false;
    bool pendingReload       = false;

    bool autoKeyWasPressed = false;
    bool pendingAutoToggle = false;

    // ── Demo Mode state ─────────────────────────────────────────────────────────
    float masterBrightness = 1.0f;
    float masterGlow       = 1.0f;
    float masterExposure   = 1.0f;
    bool  fxEnabled        = true;   // F10 toggles visual FX (glow/bloom/noise)
    bool  demoMode         = false;  // F9 toggles presentation mode

    // Fullscreen toggle bookkeeping — windowed geometry to restore.
    bool isFullscreen = false;
    int  windowedX = 100, windowedY = 100, windowedW = 1280, windowedH = 720;

    // Cursor auto-hide (demo/fullscreen): hide after a few idle seconds.
    double lastActivityTime = 0.0;
    double lastCursorX = 0.0, lastCursorY = 0.0;
    bool   cursorHidden = false;

    // Extra control edges.
    bool demoKeyWasPressed       = false; // F9
    bool fxKeyWasPressed         = false; // F10
    bool fullscreenKeyWasPressed = false; // F11

    // Soundcheck input-level meter (M).
    bool showMeter        = false;
    bool meterKeyWasPressed = false;

    // ── Drag-and-drop file (runtime audio swap) ────────────────────────────────
    std::string droppedFilePath;
    bool        pendingDroppedFile = false;

    // ── Shader metadata (for smart auto-shader matching) ────────────────────────
    struct ShaderMeta {
        int   bpmLow      = 0;
        int   bpmHigh     = 300;
        int   energyTier  = 1;   // 0=low, 1=medium, 2=high, 3=peak
        float bassBias    = 0.0f; // >0 favours bass-heavy music
        float trebleBias  = 0.0f; // >0 favours treble-heavy music
    };
    std::vector<ShaderMeta> shaderMeta;  // parallel to shaderLib (slot 1..N)

    // Smart auto-shader state.
    int   recentShaders[4]   = {-1, -1, -1, -1};
    int   recentHead         = 0;
    float smoothBass         = 0.0f;
    float smoothTreble       = 0.0f;
    float smoothBpm          = 0.0f;
};

// ──────────────────────────────────────────────────────────────────────────────
// Lifetime
// ──────────────────────────────────────────────────────────────────────────────
OpenGLRenderer::OpenGLRenderer(const bool vsyncEnabled)
    : implementation_{std::make_unique<Implementation>()},
      vsyncEnabled_{vsyncEnabled} {}

OpenGLRenderer::~OpenGLRenderer() {
    Shutdown();
}

// ──────────────────────────────────────────────────────────────────────────────
// Initialize
// ──────────────────────────────────────────────────────────────────────────────
bool OpenGLRenderer::Initialize() {
    if (initialized_) {
        return true;
    }

    // ── Window & context ────────────────────────────────────────────────────
    if (glfwInit() != GLFW_TRUE) {
        return false;
    }

    // Try progressively lower GL versions so we run on the widest range of GPUs.
    // The shaders only need GLSL 330 features, so 3.3 core is the true minimum.
    struct GlVersion { int major; int minor; };
    constexpr GlVersion kVersions[] = {{4, 6}, {4, 3}, {3, 3}};

    GLFWmonitor* windowMonitor = nullptr;
    int createW = 1280;
    int createH = 720;
    if (fullscreenRequested_) {
        if (GLFWmonitor* const monitor = glfwGetPrimaryMonitor(); monitor != nullptr) {
            if (const GLFWvidmode* const mode = glfwGetVideoMode(monitor); mode != nullptr) {
                createW = mode->width;
                createH = mode->height;
                windowMonitor = monitor;
            }
        }
    }

    for (const auto& ver : kVersions) {
        glfwDefaultWindowHints();
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, ver.major);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, ver.minor);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);

        implementation_->window = glfwCreateWindow(
            createW, createH, "PAPAGEDON Core v0.0.1", windowMonitor, nullptr);

        if (implementation_->window == nullptr && windowMonitor != nullptr) {
            implementation_->window =
                glfwCreateWindow(1280, 720, "PAPAGEDON Core v0.0.1", nullptr, nullptr);
            windowMonitor = nullptr;
        }
        if (implementation_->window != nullptr) {
            break;
        }
    }

    if (implementation_->window == nullptr) {
        glfwTerminate();
        return false;
    }
    implementation_->isFullscreen = (windowMonitor != nullptr);

    glfwMakeContextCurrent(implementation_->window);
    if (gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)) == 0) {
        glfwDestroyWindow(implementation_->window);
        implementation_->window = nullptr;
        glfwTerminate();
        return false;
    }

    // Allow disabling vsync (PAPAGEDON_VSYNC=0) to uncap the frame rate on
    // high-refresh displays — useful for hitting the 120 FPS desktop target.
    if (const char* const vsyncEnv = std::getenv("PAPAGEDON_VSYNC")) {
        vsyncEnabled_ = vsyncEnv[0] != '0';
    }
    glfwSwapInterval(vsyncEnabled_ ? 1 : 0);

    // ── Fullscreen VAO ──────────────────────────────────────────────────────
    // No vertex data is needed — the vertex shader generates positions from
    // gl_VertexID.  An empty VAO is still required by the OpenGL core profile.
    glGenVertexArrays(1, &fullscreenVAO_);

    // ── Shader library ────────────────────────────────────────────────────────
    // Slot 0: the built-in reactive shader (12 Auto-VJ forms).
    if (!shaderManager_.Compile(
            ShaderManager::DefaultVertexSource(),
            ShaderManager::DefaultFragmentSource())) {
        glDeleteVertexArrays(1, &fullscreenVAO_);
        fullscreenVAO_ = 0u;
        glfwDestroyWindow(implementation_->window);
        implementation_->window = nullptr;
        glfwTerminate();
        return false;
    }
    implementation_->shaderNames.push_back("Signature (Auto-VJ forms)");

    // Slots 1..N: premium pack shaders compiled from .frag files (Badman
    // Experience by default). A file that fails to compile is skipped so a bad
    // shader never stops the show. Override the folder with PAPAGEDON_SHADER_DIR.
    try {
        namespace fs = std::filesystem;
        const char* const dirEnv = std::getenv("PAPAGEDON_SHADER_DIR");
        const std::string dir = dirEnv != nullptr
            ? std::string(dirEnv)
            : std::string("engine/rendering/shaders");
        std::error_code ec;
        if (fs::is_directory(dir, ec)) {
            std::vector<std::string> files;
            for (const auto& entry : fs::recursive_directory_iterator(dir, ec)) {
                if (!ec && entry.is_regular_file() && entry.path().extension() == ".frag") {
                    files.push_back(entry.path().string());
                }
            }
            std::sort(files.begin(), files.end());
            for (const std::string& file : files) {
                std::ifstream in(file, std::ios::binary);
                if (!in) continue;
                std::ostringstream ss; ss << in.rdbuf();
                auto sm = std::make_unique<ShaderManager>();
                if (sm->Compile(ShaderManager::DefaultVertexSource(), ss.str().c_str())) {
                    std::string name = fs::path(file).stem().string();
                    if (name.size() > 3 && name[2] == '_' &&
                        std::isdigit(static_cast<unsigned char>(name[0]))) {
                        name = name.substr(3);
                    }
                    for (char& ch : name) if (ch == '_') ch = ' ';
                    if (!name.empty()) name[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(name[0])));

                    Implementation::ShaderMeta meta;
                    const auto stem = fs::path(file).stem().string();
                    const auto packDir = fs::path(file).parent_path().parent_path();
                    const auto metaPath = packDir / "metadata" / (stem + ".json");
                    if (std::ifstream mf(metaPath, std::ios::binary); mf) {
                        std::ostringstream ms; ms << mf.rdbuf();
                        const std::string mj = ms.str();
                        if (const auto bp = mj.find("recommendedBpmRange"); bp != std::string::npos) {
                            auto br = mj.find('[', bp);
                            if (br != std::string::npos) {
                                meta.bpmLow = std::atoi(mj.c_str() + br + 1);
                                auto comma = mj.find(',', br);
                                if (comma != std::string::npos)
                                    meta.bpmHigh = std::atoi(mj.c_str() + comma + 1);
                            }
                        }
                        const std::string elv = JsonString(mj, "energyLevel");
                        if (elv == "low")         meta.energyTier = 0;
                        else if (elv == "medium")  meta.energyTier = 1;
                        else if (elv == "high")    meta.energyTier = 2;
                        else if (elv == "peak")    meta.energyTier = 3;
                        if (JsonArrayContains(mj, "mood", "atmospheric") ||
                            JsonArrayContains(mj, "mood", "cinematic"))
                            meta.bassBias += 0.3f;
                        if (JsonArrayContains(mj, "mood", "hypnotic") ||
                            JsonArrayContains(mj, "mood", "luxury"))
                            meta.bassBias += 0.2f;
                        if (JsonArrayContains(mj, "mood", "aggressive") ||
                            JsonArrayContains(mj, "mood", "minimal"))
                            meta.trebleBias += 0.3f;
                        if (JsonArrayContains(mj, "mood", "dark"))
                            meta.bassBias += 0.1f;
                    }

                    implementation_->shaderLib.push_back(std::move(sm));
                    implementation_->shaderNames.push_back(name);
                    implementation_->shaderMeta.push_back(meta);
                } else {
                    std::fprintf(stderr, "[ShaderLib] skipped (compile failed): %s\n", file.c_str());
                }
            }
        }
    } catch (...) {
        std::fprintf(stderr, "[ShaderLib] shader directory scan failed — continuing with built-in shader only.\n");
    }

    // Dramatic transition FBOs + shader (render-to-texture crossfade).
    // The two FBOs start at 0×0 — they are (re)allocated to match the framebuffer
    // on first use and whenever the window resizes.
    glGenFramebuffers(2, implementation_->transitionFBO);
    glGenTextures(2, implementation_->transitionTex);
    for (int i = 0; i < 2; ++i) {
        glBindTexture(GL_TEXTURE_2D, implementation_->transitionTex[i]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glBindFramebuffer(GL_FRAMEBUFFER, implementation_->transitionFBO[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_2D, implementation_->transitionTex[i], 0);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);

    implementation_->transitionProgram =
        CompileGLProgram(ShaderManager::DefaultVertexSource(), kDeformationTransitionFragment);
    if (implementation_->transitionProgram != 0) {
        implementation_->txLocTexA       = glGetUniformLocation(implementation_->transitionProgram, "uTexA");
        implementation_->txLocTexB       = glGetUniformLocation(implementation_->transitionProgram, "uTexB");
        implementation_->txLocProgress   = glGetUniformLocation(implementation_->transitionProgram, "uProgress");
        implementation_->txLocEffect     = glGetUniformLocation(implementation_->transitionProgram, "uEffect");
        implementation_->txLocTime       = glGetUniformLocation(implementation_->transitionProgram, "uTime");
        implementation_->txLocResolution = glGetUniformLocation(implementation_->transitionProgram, "uResolution");
    }
    implementation_->lastSwitchTime = glfwGetTime();

    // PGX visual-memory FBOs.  Textures are allocated lazily to the framebuffer
    // size in Render(), just like transition textures.
    glGenFramebuffers(1, &implementation_->pgxSceneFBO);
    glGenTextures(1, &implementation_->pgxSceneTex);
    glBindTexture(GL_TEXTURE_2D, implementation_->pgxSceneTex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindFramebuffer(GL_FRAMEBUFFER, implementation_->pgxSceneFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D, implementation_->pgxSceneTex, 0);

    glGenFramebuffers(2, implementation_->pgxFeedbackFBO);
    glGenTextures(2, implementation_->pgxFeedbackTex);
    for (int i = 0; i < 2; ++i) {
        glBindTexture(GL_TEXTURE_2D, implementation_->pgxFeedbackTex[i]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glBindFramebuffer(GL_FRAMEBUFFER, implementation_->pgxFeedbackFBO[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_2D, implementation_->pgxFeedbackTex[i], 0);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);

    implementation_->pgxFeedbackProgram =
        CompileGLProgram(ShaderManager::DefaultVertexSource(), kPgxFeedbackFragment);
    if (implementation_->pgxFeedbackProgram != 0) {
        implementation_->pgxLocScene =
            glGetUniformLocation(implementation_->pgxFeedbackProgram, "uScene");
        implementation_->pgxLocPrevious =
            glGetUniformLocation(implementation_->pgxFeedbackProgram, "uPrevious");
        implementation_->pgxLocTime =
            glGetUniformLocation(implementation_->pgxFeedbackProgram, "uTime");
        implementation_->pgxLocResolution =
            glGetUniformLocation(implementation_->pgxFeedbackProgram, "uResolution");
        implementation_->pgxLocDecay =
            glGetUniformLocation(implementation_->pgxFeedbackProgram, "uDecay");
        implementation_->pgxLocZoom =
            glGetUniformLocation(implementation_->pgxFeedbackProgram, "uZoom");
        implementation_->pgxLocRotation =
            glGetUniformLocation(implementation_->pgxFeedbackProgram, "uRotation");
        implementation_->pgxLocWarp =
            glGetUniformLocation(implementation_->pgxFeedbackProgram, "uWarp");
        implementation_->pgxLocBeatWarp =
            glGetUniformLocation(implementation_->pgxFeedbackProgram, "uBeatWarp");
        implementation_->pgxLocBeat =
            glGetUniformLocation(implementation_->pgxFeedbackProgram, "uBeat");
        implementation_->pgxLocEnergy =
            glGetUniformLocation(implementation_->pgxFeedbackProgram, "uEnergy");
    }

    glGenTextures(1, &implementation_->pgxSpectrumTex);
    glBindTexture(GL_TEXTURE_2D, implementation_->pgxSpectrumTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F,
                 static_cast<GLsizei>(audio::kSpectrumBandCount), 1, 0,
                 GL_RED, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glGenTextures(1, &implementation_->pgxWaveformTex);
    glBindTexture(GL_TEXTURE_2D, implementation_->pgxWaveformTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F,
                 static_cast<GLsizei>(audio::kWaveformSampleCount), 1, 0,
                 GL_RED, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    implementation_->pgxAudioShapeProgram =
        CompileGLProgram(ShaderManager::DefaultVertexSource(), kPgxAudioShapeFragment);
    if (implementation_->pgxAudioShapeProgram != 0) {
        const unsigned int p = implementation_->pgxAudioShapeProgram;
        implementation_->pgxShapeLocSpectrum =
            glGetUniformLocation(p, "uSpectrum");
        implementation_->pgxShapeLocWaveform =
            glGetUniformLocation(p, "uWaveform");
        implementation_->pgxShapeLocTime =
            glGetUniformLocation(p, "uTime");
        implementation_->pgxShapeLocResolution =
            glGetUniformLocation(p, "uResolution");
        implementation_->pgxShapeLocWaveformMode =
            glGetUniformLocation(p, "uWaveformMode");
        implementation_->pgxShapeLocWaveformOpacity =
            glGetUniformLocation(p, "uWaveformOpacity");
        implementation_->pgxShapeLocWaveformThickness =
            glGetUniformLocation(p, "uWaveformThickness");
        implementation_->pgxShapeLocWaveformRadius =
            glGetUniformLocation(p, "uWaveformRadius");
        implementation_->pgxShapeLocWaveformBassResponse =
            glGetUniformLocation(p, "uWaveformBassResponse");
        implementation_->pgxShapeLocWaveformTrebleResponse =
            glGetUniformLocation(p, "uWaveformTrebleResponse");
        implementation_->pgxShapeLocBass =
            glGetUniformLocation(p, "uBass");
        implementation_->pgxShapeLocMid =
            glGetUniformLocation(p, "uMid");
        implementation_->pgxShapeLocTreble =
            glGetUniformLocation(p, "uTreble");
        implementation_->pgxShapeLocBeat =
            glGetUniformLocation(p, "uBeat");
        implementation_->pgxShapeLocEnergy =
            glGetUniformLocation(p, "uEnergy");
        implementation_->pgxShapeLocPhase =
            glGetUniformLocation(p, "uPhase");
        implementation_->pgxShapeLocSeed =
            glGetUniformLocation(p, "uSeed");
        implementation_->pgxShapeLocPrimary =
            glGetUniformLocation(p, "uPrimaryColour");
        implementation_->pgxShapeLocSecondary =
            glGetUniformLocation(p, "uSecondaryColour");
        implementation_->pgxShapeLocAccent =
            glGetUniformLocation(p, "uAccentColour");
    }

    // Cinematic startup logo program. PAPAGEDON_NO_INTRO=1 skips it (fast relaunch).
    implementation_->introProgram = CompileGLProgram(ShaderManager::DefaultVertexSource(), kIntroFragment);
    implementation_->introTimeLoc = implementation_->introProgram != 0
        ? glGetUniformLocation(implementation_->introProgram, "uTime") : -1;
    implementation_->introResLoc = implementation_->introProgram != 0
        ? glGetUniformLocation(implementation_->introProgram, "uResolution") : -1;
    if (const char* const noIntro = std::getenv("PAPAGEDON_NO_INTRO")) {
        if (noIntro[0] != '0') {
            implementation_->introActive = false;
        }
    }
    if (implementation_->introProgram == 0) {
        implementation_->introActive = false; // no program → straight to the show
    }

    // Auto-shader director: transition through the library with the music.
    if (const char* const autoEnv = std::getenv("PAPAGEDON_AUTO_SHADER")) {
        implementation_->autoShader = autoEnv[0] != '0';
    }

    // Launch straight into the soundcheck level meter when requested.
    if (const char* const meterEnv = std::getenv("PAPAGEDON_METER")) {
        implementation_->showMeter = meterEnv[0] != '0';
    }

    // Optional starting shader (index into the library: 0 = signature, 1..N = pack).
    if (const char* const startShader = std::getenv("PAPAGEDON_START_SHADER")) {
        const int idx = std::atoi(startShader);
        const int count = 1 + static_cast<int>(implementation_->shaderLib.size());
        if (idx >= 0 && idx < count) {
            implementation_->activeShader = idx;
        }
    }

    // ── Drag-and-drop audio files ───────────────────────────────────────────
    glfwSetWindowUserPointer(implementation_->window, implementation_.get());
    glfwSetDropCallback(implementation_->window,
        [](GLFWwindow* w, int count, const char** paths) {
            if (count < 1 || paths == nullptr || paths[0] == nullptr) return;
            auto* impl = static_cast<Implementation*>(glfwGetWindowUserPointer(w));
            if (impl == nullptr) return;
            const std::string path = paths[0];
            const auto ext = std::filesystem::path(path).extension().string();
            std::string lower;
            lower.reserve(ext.size());
            for (char c : ext)
                lower += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            if (lower == ".wav" || lower == ".mp3" || lower == ".flac" || lower == ".ogg") {
                impl->droppedFilePath    = path;
                impl->pendingDroppedFile = true;
            }
        });

    // ── Debug overlay ────────────────────────────────────────────────────────
    implementation_->lastFpsUpdateTime  = glfwGetTime();
    implementation_->lastFrameTime      = glfwGetTime();
    implementation_->renderedFrameCount = 0;
    implementation_->debugOverlay.Initialize();

    // Cursor-activity baseline for demo-mode auto-hide.
    implementation_->lastActivityTime = glfwGetTime();
    glfwGetCursorPos(implementation_->window,
                     &implementation_->lastCursorX, &implementation_->lastCursorY);

    initialized_ = true;
    return true;
}

// ──────────────────────────────────────────────────────────────────────────────
// Demo-mode configuration
// ──────────────────────────────────────────────────────────────────────────────
void OpenGLRenderer::Configure(const bool fullscreen, const bool vsync) {
    fullscreenRequested_ = fullscreen;
    vsyncEnabled_        = vsync;
}

void OpenGLRenderer::SetMasterControls(const float brightness, const float glow,
                                       const float exposure) {
    implementation_->masterBrightness = brightness;
    implementation_->masterGlow       = glow;
    implementation_->masterExposure   = exposure;
}

void OpenGLRenderer::SetDemoMode(const bool enabled) {
    implementation_->demoMode = enabled;
    if (implementation_->window == nullptr) {
        return;
    }
    if (enabled) {
        implementation_->showDebugOverlay = false;
    } else {
        glfwSetInputMode(implementation_->window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        implementation_->cursorHidden = false;
    }
    implementation_->lastActivityTime = glfwGetTime();
}

void OpenGLRenderer::SetDebugOverlay(const bool visible) {
    implementation_->showDebugOverlay = visible;
}

const char* OpenGLRenderer::BackendName() const noexcept {
    return "OpenGL 4.6 Core";
}

// ──────────────────────────────────────────────────────────────────────────────
// PresentSplash — one branded loading frame
// ──────────────────────────────────────────────────────────────────────────────
void OpenGLRenderer::PresentSplash(const std::string& status, const float progress) {
    if (!initialized_ || implementation_->window == nullptr) {
        return;
    }
    int width = 0;
    int height = 0;
    glfwGetFramebufferSize(implementation_->window, &width, &height);
    glViewport(0, 0, width, height);
    glClearColor(0.018F, 0.050F, 0.032F, 1.0F); // deep forest green (brand)
    glClear(GL_COLOR_BUFFER_BIT);

    implementation_->debugOverlay.RenderSplash(
        "PAPAGEDON Core  v0.0.1", status.c_str(), progress, width, height);

    glfwSwapBuffers(implementation_->window);
    glfwPollEvents();
}

// ──────────────────────────────────────────────────────────────────────────────
// SetFullscreen
// ──────────────────────────────────────────────────────────────────────────────
void OpenGLRenderer::SetFullscreen(const bool enable) {
    if (implementation_->window == nullptr || enable == implementation_->isFullscreen) {
        return;
    }
    if (enable) {
        glfwGetWindowPos(implementation_->window,
                         &implementation_->windowedX, &implementation_->windowedY);
        glfwGetWindowSize(implementation_->window,
                          &implementation_->windowedW, &implementation_->windowedH);
        // Find the monitor the window is currently on (centre-point test) so that
        // F11 goes fullscreen on the correct display — including secondary screens.
        GLFWmonitor* bestMonitor = nullptr;
        {
            int wx, wy, ww, wh;
            glfwGetWindowPos(implementation_->window, &wx, &wy);
            glfwGetWindowSize(implementation_->window, &ww, &wh);
            const int cx = wx + ww / 2;
            const int cy = wy + wh / 2;
            int monitorCount = 0;
            GLFWmonitor** monitors = glfwGetMonitors(&monitorCount);
            for (int i = 0; i < monitorCount; ++i) {
                int mx, my;
                glfwGetMonitorPos(monitors[i], &mx, &my);
                const GLFWvidmode* vm = glfwGetVideoMode(monitors[i]);
                if (vm && cx >= mx && cx < mx + vm->width &&
                    cy >= my && cy < my + vm->height) {
                    bestMonitor = monitors[i];
                    break;
                }
            }
            if (!bestMonitor) bestMonitor = glfwGetPrimaryMonitor();
        }
        const GLFWvidmode* const mode = bestMonitor ? glfwGetVideoMode(bestMonitor) : nullptr;
        if (mode == nullptr) {
            return;
        }
        glfwSetWindowMonitor(implementation_->window, bestMonitor, 0, 0,
                             mode->width, mode->height, mode->refreshRate);
        implementation_->isFullscreen = true;
    } else {
        glfwSetWindowMonitor(implementation_->window, nullptr,
                             implementation_->windowedX, implementation_->windowedY,
                             implementation_->windowedW, implementation_->windowedH, 0);
        implementation_->isFullscreen = false;
    }
    glfwSwapInterval(vsyncEnabled_ ? 1 : 0);
}

// ──────────────────────────────────────────────────────────────────────────────
// BeginShaderTransition — dramatic randomised crossfade between two shaders
// ──────────────────────────────────────────────────────────────────────────────
void OpenGLRenderer::BeginShaderTransition(const int target) {
    const int count = 1 + static_cast<int>(implementation_->shaderLib.size());
    if (target < 0 || target >= count) {
        return;
    }
    if (target == implementation_->activeShader) {
        return;
    }

    // Pick a random dramatic effect, avoiding the same one twice in a row.
    const int ticks = static_cast<int>(glfwGetTime() * 100000.0);
    int effect = ((ticks ^ (ticks >> 5)) * 2654435761u) % kNumTransitionEffects;
    if (effect == implementation_->lastTransitionEffect && kNumTransitionEffects > 1) {
        effect = (effect + 1 + (ticks % (kNumTransitionEffects - 1))) % kNumTransitionEffects;
    }
    implementation_->lastTransitionEffect = effect;
    implementation_->transitionEffect     = effect;

    implementation_->outgoingShader       = implementation_->activeShader;
    implementation_->pendingShader        = target;
    implementation_->transitioning        = true;
    implementation_->transitionProgress   = 0.0f;
    implementation_->needsOutgoingCapture = true;

    implementation_->toastText  = implementation_->shaderNames[static_cast<std::size_t>(target)];
    implementation_->toastUntil = glfwGetTime() + 2.2;
}

// ──────────────────────────────────────────────────────────────────────────────
// BeginFrame
// ──────────────────────────────────────────────────────────────────────────────
void OpenGLRenderer::BeginFrame() {
    // Reserved for future pre-frame GPU work (e.g. fence waits, UBO updates).
}

// ──────────────────────────────────────────────────────────────────────────────
// Render
// ──────────────────────────────────────────────────────────────────────────────
void OpenGLRenderer::Render(
    const SceneState&     state,
    const DebugState&     debugState,
    const audio::ExperienceSignals& signals,
    const ExperiencePreset& preset,
    const visual::Theme&  theme) {

    if (!initialized_) {
        return;
    }

    // ── Cinematic startup intro ─────────────────────────────────────────────────
    // Plays once before the show: the animated gold logo + wordmark on deep green.
    // Draws to the back buffer; EndFrame swaps and still polls input (so ESC quits
    // and ENTER can skip).  Returns early so the show doesn't render underneath.
    if (implementation_->introActive) {
        const double now = glfwGetTime();
        if (implementation_->introStart < 0.0) {
            implementation_->introStart = now;
        }
        const float introT = static_cast<float>(now - implementation_->introStart);
        constexpr float kIntroDuration = 4.9f;

        int iw = 0;
        int ih = 0;
        glfwGetFramebufferSize(implementation_->window, &iw, &ih);
        glViewport(0, 0, iw, ih);
        glClearColor(0.018F, 0.050F, 0.032F, 1.0F); // deep forest green
        glClear(GL_COLOR_BUFFER_BIT);

        if (implementation_->introProgram != 0) {
            glUseProgram(implementation_->introProgram);
            if (implementation_->introTimeLoc >= 0) {
                glUniform1f(implementation_->introTimeLoc, introT);
            }
            if (implementation_->introResLoc >= 0) {
                glUniform2f(implementation_->introResLoc, static_cast<float>(iw), static_cast<float>(ih));
            }
            glBindVertexArray(fullscreenVAO_);
            glDrawArrays(GL_TRIANGLES, 0, 3);
            glBindVertexArray(0u);
        }

        // Wordmark fades in under the mark, then out with the whole intro.
        const float wordIn  = std::clamp((introT - 2.7f) / 1.0f, 0.0f, 1.0f);
        const float wordOut = 1.0f - std::clamp((introT - 4.35f) / 0.55f, 0.0f, 1.0f);
        implementation_->debugOverlay.RenderBrandWordmark(iw, ih, wordIn * wordOut);

        // ENTER skips the intro.
        const bool skip = glfwGetKey(implementation_->window, GLFW_KEY_ENTER) == GLFW_PRESS;
        if (introT >= kIntroDuration || skip) {
            implementation_->introActive = false;
            implementation_->lastFrameTime = now;       // avoid a huge first dt
            implementation_->lastSwitchTime = now;      // don't auto-switch instantly
            implementation_->menuVisibleUntil = now + 9.0; // auto-show controls once
        }
        return;
    }

    const double currentTime = glfwGetTime();
    const float deltaTime = static_cast<float>(currentTime - implementation_->lastFrameTime);
    implementation_->lastFrameTime = currentTime;

    auto& smoothed = implementation_->smoothedUniforms;

    // ── Audio-signal following ─────────────────────────────────────────────────
    // Low visual latency is a primary target, so transients use a fast attack
    // (~13 ms — an onset lands within a frame) and a short release (~83 ms)
    // to keep the decay smooth.  This keeps the image locked to the beat instead
    // of trailing it, while still avoiding per-frame strobing.
    constexpr float kAttack  = 80.0f; // rise time constant ~13 ms
    constexpr float kRelease = 12.0f; // fall time constant ~83 ms

    // The preset scales how much energy drives brightness.
    const float targetEnergy = std::clamp(signals.energy * preset.energyMultiplier, 0.0f, 1.0f);
    smoothed.energy    = Follow(smoothed.energy,    targetEnergy,      deltaTime, kAttack, kRelease);
    smoothed.intensity = Follow(smoothed.intensity, signals.intensity, deltaTime, kAttack, kRelease);
    smoothed.bass      = Follow(smoothed.bass,      signals.bass,      deltaTime, kAttack, kRelease);
    smoothed.mid       = Follow(smoothed.mid,       signals.mid,       deltaTime, kAttack, kRelease);
    smoothed.treble    = Follow(smoothed.treble,    signals.treble,    deltaTime, kAttack, kRelease);

    // Beat pulse: instant on the beat frame (zero latency), then a fast decay so
    // it reads as a punch.  Peaks at the preset's beat response.
    if (signals.beat) {
        smoothed.beat = preset.beatResponse;
    } else {
        const float beatDecayRate = 14.0f; // exp(-dt * 14) gives ~25% after 100 ms
        smoothed.beat *= std::exp(-deltaTime * beatDecayRate);
    }

    // Mood is a slow scene property, not a transient — follow it gently.
    smoothed.mood = Follow(smoothed.mood, state.mood, deltaTime, 6.0f, 6.0f);

    // ── Theme colour identity & look ────────────────────────────────────────────
    // Colour, glow, bloom, motion, noise and distortion all come from the active
    // visual::Theme — this is what makes switching themes transform the whole
    // visual identity while the same music plays.  The palette maps
    // secondary → primary → accent across the pattern's dark → bright ramp, with
    // `background` filling the darkest regions.  The theme's transitionSpeed sets
    // the ease rate, so a switch cross-fades the whole image.  Every update is a
    // few float lerps on the persistent smoothed uniforms — a theme switch
    // performs no heap allocation.  All easing is frame-rate independent.
    const float themeAlpha =
        1.0f - std::exp(-deltaTime * std::max(theme.transitionSpeed, 0.0f));

    smoothed.primaryColor   = LerpColor(smoothed.primaryColor,   ToColor3(theme.palette.primary),    themeAlpha);
    smoothed.secondaryColor = LerpColor(smoothed.secondaryColor, ToColor3(theme.palette.secondary),  themeAlpha);
    smoothed.accentColor    = LerpColor(smoothed.accentColor,    ToColor3(theme.palette.accent),     themeAlpha);
    smoothed.background     = LerpColor(smoothed.background,     ToColor3(theme.palette.background), themeAlpha);
    // Visual FX (F10) gate glow/bloom/noise; when off they ease smoothly to zero,
    // leaving the clean base pattern.  Motion and distortion are core animation,
    // not "FX", so they are unaffected.
    const float fx = implementation_->fxEnabled ? 1.0f : 0.0f;
    smoothed.glow       += (theme.glow  * fx    - smoothed.glow)       * themeAlpha;
    smoothed.bloom      += (theme.bloom * fx    - smoothed.bloom)      * themeAlpha;
    smoothed.motion     += (theme.motion        - smoothed.motion)     * themeAlpha;
    smoothed.noise      += (theme.noise * fx    - smoothed.noise)      * themeAlpha;
    smoothed.distortion += (theme.distortion    - smoothed.distortion) * themeAlpha;

    // Master output trims (Demo Mode operator globals) — applied directly.
    smoothed.masterBrightness = implementation_->masterBrightness;
    smoothed.masterGlow       = implementation_->masterGlow;
    smoothed.masterExposure   = implementation_->masterExposure;

    // ── Preset form & behaviour ─────────────────────────────────────────────────
    // The preset owns the *form* axis — which signature pattern is drawn and its
    // saturation / detail — orthogonal to the theme's colour identity.  Its
    // transitionSpeed eases these form parameters across a preset switch.
    const float presetAlpha =
        1.0f - std::exp(-deltaTime * std::max(preset.transitionSpeed, 0.0f));

    smoothed.saturationBase  += (preset.saturationBase  - smoothed.saturationBase)  * presetAlpha;
    smoothed.saturationScale += (preset.saturationScale - smoothed.saturationScale) * presetAlpha;
    smoothed.detail          += (preset.detail          - smoothed.detail)          * presetAlpha;

    // ── PGX runtime controls ──────────────────────────────────────────────────
    // PGX is PAPAGEDON's native visual-runtime contract.  These values are data
    // supplied by the preset and eased here so future feedback/waveform passes
    // can consume stable, pop-free controls.
    smoothed.feedbackDecay    += (preset.pgx.feedback.decay    - smoothed.feedbackDecay)    * presetAlpha;
    smoothed.feedbackZoom     += (preset.pgx.feedback.zoom     - smoothed.feedbackZoom)     * presetAlpha;
    smoothed.feedbackRotation += (preset.pgx.feedback.rotation - smoothed.feedbackRotation) * presetAlpha;
    smoothed.feedbackWarp     += (preset.pgx.feedback.warp     - smoothed.feedbackWarp)     * presetAlpha;
    smoothed.feedbackBeatWarp += (preset.pgx.feedback.beatWarp - smoothed.feedbackBeatWarp) * presetAlpha;
    smoothed.waveformMode      = static_cast<int>(preset.pgx.waveform.mode);
    smoothed.waveformOpacity  += (preset.pgx.waveform.opacity  - smoothed.waveformOpacity)  * presetAlpha;
    smoothed.waveformThickness += (preset.pgx.waveform.thickness - smoothed.waveformThickness) * presetAlpha;
    smoothed.waveformRadius   += (preset.pgx.waveform.radius   - smoothed.waveformRadius)   * presetAlpha;
    smoothed.waveformBassResponse +=
        (preset.pgx.waveform.bassResponse - smoothed.waveformBassResponse) * presetAlpha;
    smoothed.waveformTrebleResponse +=
        (preset.pgx.waveform.trebleResponse - smoothed.waveformTrebleResponse) * presetAlpha;

    // Preset-local PGX state: a persistent phase and seed give the renderer
    // enough memory to evolve shape layers over minutes instead of locking every
    // preset to a short deterministic loop.
    if (!implementation_->pgxStateInitialized || implementation_->pgxStatePreset != preset.id) {
        implementation_->pgxStatePreset = preset.id;
        implementation_->pgxStateInitialized = true;
        const auto id = static_cast<std::size_t>(preset.id);
        implementation_->pgxSeed = 17.0f + static_cast<float>(id * 37u);
        implementation_->pgxPhase *= 0.35f;
    }
    implementation_->pgxPhase += deltaTime *
        (0.35f + smoothed.energy * 1.6f + smoothed.bass * 0.8f + smoothed.beat * 2.0f);
    smoothed.evolutionPhase = implementation_->pgxPhase;

    // ── Signature form cross-fade ──────────────────────────────────────────────
    // The pattern is a discrete choice, so it can't be lerped like a colour.
    // Instead we snapshot the outgoing form and ease patternBlend 0 → 1; the
    // shader mixes the two forms while the blend runs, then draws only the active
    // form once it completes.  This is state juggling only — no allocation.
    const int incomingMode = static_cast<int>(preset.patternMode);
    if (!implementation_->patternInitialized) {
        implementation_->activePatternMode   = incomingMode;
        implementation_->previousPatternMode = incomingMode;
        implementation_->patternBlend        = 1.0f;
        implementation_->patternInitialized  = true;
    } else if (incomingMode != implementation_->activePatternMode) {
        implementation_->previousPatternMode = implementation_->activePatternMode;
        implementation_->activePatternMode   = incomingMode;
        implementation_->patternBlend        = 0.0f;
    }
    implementation_->patternBlend += (1.0f - implementation_->patternBlend) * presetAlpha;
    if (implementation_->patternBlend > 0.999f) {
        implementation_->patternBlend        = 1.0f;
        implementation_->previousPatternMode = implementation_->activePatternMode;
    }
    smoothed.patternMode         = implementation_->activePatternMode;
    smoothed.previousPatternMode = implementation_->previousPatternMode;
    smoothed.patternBlend        = implementation_->patternBlend;

    // Query framebuffer size for the uResolution uniform and viewport.
    int width  = 0;
    int height = 0;
    glfwGetFramebufferSize(implementation_->window, &width, &height);
    glViewport(0, 0, width, height);

    // Clear to black — the fullscreen triangle overwrites this, but the clear
    // is kept for correctness on framebuffers with a depth/stencil attachment.
    glClearColor(0.0F, 0.0F, 0.0F, 1.0F);
    glClear(GL_COLOR_BUFFER_BIT);

    // Auto-shader: move through the library with the music, matching shaders
    // to the current audio character (BPM, energy, frequency profile).
    if (implementation_->autoShader) {
        implementation_->prevBeat = signals.beat;
        const float ba = 1.0f - std::exp(-deltaTime * 0.4f);
        implementation_->energyBaseline += (signals.energy - implementation_->energyBaseline) * ba;
        const float sa = 1.0f - std::exp(-deltaTime * 0.8f);
        implementation_->smoothBass   += (signals.bass   - implementation_->smoothBass)   * sa;
        implementation_->smoothTreble += (signals.treble  - implementation_->smoothTreble) * sa;
        if (signals.bpm > 30.0f) {
            const float bpmAlpha = 1.0f - std::exp(-deltaTime * 0.3f);
            implementation_->smoothBpm += (signals.bpm - implementation_->smoothBpm) * bpmAlpha;
        }
        const double since  = currentTime - implementation_->lastSwitchTime;
        const bool   drop   = (signals.energy - implementation_->energyBaseline) > 0.28f && signals.energy > 0.45f;
        const bool   timeUp = since > 8.0;
        if (!implementation_->transitioning && since > 3.0 && (drop || timeUp)) {
            const int count = 1 + static_cast<int>(implementation_->shaderLib.size());
            if (count > 1 && !implementation_->shaderMeta.empty()) {
                const float bpm    = implementation_->smoothBpm;
                const float energy = implementation_->energyBaseline;
                const float bass   = implementation_->smoothBass;
                const float treble = implementation_->smoothTreble;
                const int eTier = energy < 0.25f ? 0 : energy < 0.50f ? 1
                                : energy < 0.75f ? 2 : 3;
                float bestScore  = -1e9f;
                int   bestShader = (implementation_->activeShader + 1) % count;
                for (int i = 1; i < count; ++i) {
                    const std::size_t mi = static_cast<std::size_t>(i - 1);
                    if (mi >= implementation_->shaderMeta.size()) continue;
                    const auto& m = implementation_->shaderMeta[mi];
                    float score = 0.0f;
                    if (bpm > 30.0f && m.bpmLow > 0) {
                        if (bpm >= static_cast<float>(m.bpmLow) &&
                            bpm <= static_cast<float>(m.bpmHigh)) {
                            score += 3.0f;
                        } else {
                            const float dist = bpm < static_cast<float>(m.bpmLow)
                                ? static_cast<float>(m.bpmLow) - bpm
                                : bpm - static_cast<float>(m.bpmHigh);
                            score -= dist * 0.15f;
                        }
                    }
                    const int tierDist = std::abs(eTier - m.energyTier);
                    score += 2.0f - static_cast<float>(tierDist) * 1.0f;
                    const float freqBalance = bass - treble;
                    score += freqBalance * m.bassBias * 3.0f;
                    score -= freqBalance * m.trebleBias * 2.0f;
                    score += (treble - bass) * m.trebleBias * 3.0f;
                    for (int r : implementation_->recentShaders) {
                        if (r == i) { score -= 5.0f; break; }
                    }
                    const float jitter = static_cast<float>((i * 7 + static_cast<int>(currentTime * 3.0)) % 100) * 0.005f;
                    score += jitter;
                    if (score > bestScore) {
                        bestScore  = score;
                        bestShader = i;
                    }
                }
                implementation_->recentShaders[implementation_->recentHead % 4] = bestShader;
                implementation_->recentHead++;
                BeginShaderTransition(bestShader);
            } else {
                BeginShaderTransition((implementation_->activeShader + 1) % count);
            }
            implementation_->lastSwitchTime = currentTime;
        }
    }

    // ── Fullscreen shader pass (with dramatic FBO crossfade transitions) ──
    const float time = static_cast<float>(currentTime);

    glBindTexture(GL_TEXTURE_2D, implementation_->pgxSpectrumTex);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
                    static_cast<GLsizei>(audio::kSpectrumBandCount), 1,
                    GL_RED, GL_FLOAT, signals.spectrum.data());
    glBindTexture(GL_TEXTURE_2D, implementation_->pgxWaveformTex);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
                    static_cast<GLsizei>(audio::kWaveformSampleCount), 1,
                    GL_RED, GL_FLOAT, signals.waveform.data());
    glBindTexture(GL_TEXTURE_2D, 0);

    // Ensure transition FBO textures match the current framebuffer dimensions.
    if (width != implementation_->transitionTexW ||
        height != implementation_->transitionTexH) {
        implementation_->transitionTexW = width;
        implementation_->transitionTexH = height;
        for (int i = 0; i < 2; ++i) {
            glBindTexture(GL_TEXTURE_2D, implementation_->transitionTex[i]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0,
                         GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        }
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    if (width != implementation_->pgxTexW ||
        height != implementation_->pgxTexH) {
        implementation_->pgxTexW = width;
        implementation_->pgxTexH = height;

        glBindTexture(GL_TEXTURE_2D, implementation_->pgxSceneTex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

        for (int i = 0; i < 2; ++i) {
            glBindTexture(GL_TEXTURE_2D, implementation_->pgxFeedbackTex[i]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0,
                         GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
            glBindFramebuffer(GL_FRAMEBUFFER, implementation_->pgxFeedbackFBO[i]);
            glViewport(0, 0, width, height);
            glClearColor(0.0F, 0.0F, 0.0F, 1.0F);
            glClear(GL_COLOR_BUFFER_BIT);
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glBindTexture(GL_TEXTURE_2D, 0);
        implementation_->pgxReadIndex = 0;
        implementation_->pgxFeedbackPrimed = false;
    }

    if (implementation_->transitioning && implementation_->transitionProgram != 0) {
        // ── Capture outgoing shader to FBO-A (once, on the first frame) ──
        if (implementation_->needsOutgoingCapture) {
            glBindFramebuffer(GL_FRAMEBUFFER, implementation_->transitionFBO[0]);
            glViewport(0, 0, width, height);
            glClear(GL_COLOR_BUFFER_BIT);

            const int outIdx = implementation_->outgoingShader;
            ShaderManager& outgoing = (outIdx == 0)
                ? shaderManager_
                : *implementation_->shaderLib[outIdx - 1];
            outgoing.Bind();
            outgoing.SetUniforms(smoothed, time, width, height);
            glBindVertexArray(fullscreenVAO_);
            glDrawArrays(GL_TRIANGLES, 0, 3);
            glBindVertexArray(0u);

            implementation_->activeShader = implementation_->pendingShader;
            implementation_->needsOutgoingCapture = false;
        }

        // ── Render incoming shader to FBO-B (every frame — live with music) ──
        glBindFramebuffer(GL_FRAMEBUFFER, implementation_->transitionFBO[1]);
        glViewport(0, 0, width, height);
        glClear(GL_COLOR_BUFFER_BIT);

        ShaderManager& incoming = (implementation_->activeShader == 0)
            ? shaderManager_
            : *implementation_->shaderLib[implementation_->activeShader - 1];
        incoming.Bind();
        incoming.SetUniforms(smoothed, time, width, height);
        glBindVertexArray(fullscreenVAO_);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glBindVertexArray(0u);

        // ── Blend with the dramatic transition shader into the PGX scene ──
        glBindFramebuffer(GL_FRAMEBUFFER, implementation_->pgxSceneFBO);
        glViewport(0, 0, width, height);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(implementation_->transitionProgram);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, implementation_->transitionTex[0]);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, implementation_->transitionTex[1]);
        if (implementation_->txLocTexA >= 0)
            glUniform1i(implementation_->txLocTexA, 0);
        if (implementation_->txLocTexB >= 0)
            glUniform1i(implementation_->txLocTexB, 1);
        if (implementation_->txLocProgress >= 0)
            glUniform1f(implementation_->txLocProgress, implementation_->transitionProgress);
        if (implementation_->txLocEffect >= 0)
            glUniform1i(implementation_->txLocEffect, implementation_->transitionEffect);
        if (implementation_->txLocTime >= 0)
            glUniform1f(implementation_->txLocTime, time);
        if (implementation_->txLocResolution >= 0)
            glUniform2f(implementation_->txLocResolution,
                        static_cast<float>(width), static_cast<float>(height));

        glBindVertexArray(fullscreenVAO_);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glBindVertexArray(0u);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, 0);

        // Advance progress — each effect has its own duration.
        const int eff = implementation_->transitionEffect;
        const float dur = (eff >= 0 && eff < kNumTransitionEffects)
            ? kTransitionDurations[eff] : 0.8f;
        implementation_->transitionProgress += deltaTime / dur;
        if (implementation_->transitionProgress >= 1.0f) {
            implementation_->transitionProgress = 1.0f;
            implementation_->transitioning = false;
        }
    } else {
        // ── Normal rendering into the PGX scene (no transition active) ──
        glBindFramebuffer(GL_FRAMEBUFFER, implementation_->pgxSceneFBO);
        glViewport(0, 0, width, height);
        glClear(GL_COLOR_BUFFER_BIT);

        ShaderManager& active = (implementation_->activeShader == 0)
            ? shaderManager_
            : *implementation_->shaderLib[implementation_->activeShader - 1];
        active.Bind();
        active.SetUniforms(smoothed, time, width, height);

        glBindVertexArray(fullscreenVAO_);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glBindVertexArray(0u);
    }

    // ── PGX visual-memory composite ───────────────────────────────────────────
    if (implementation_->pgxFeedbackProgram != 0) {
        const int readIndex = implementation_->pgxReadIndex;
        const int writeIndex = 1 - readIndex;

        glBindFramebuffer(GL_FRAMEBUFFER, implementation_->pgxFeedbackFBO[writeIndex]);
        glViewport(0, 0, width, height);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(implementation_->pgxFeedbackProgram);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, implementation_->pgxSceneTex);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, implementation_->pgxFeedbackTex[readIndex]);
        if (implementation_->pgxLocScene >= 0)
            glUniform1i(implementation_->pgxLocScene, 0);
        if (implementation_->pgxLocPrevious >= 0)
            glUniform1i(implementation_->pgxLocPrevious, 1);
        if (implementation_->pgxLocTime >= 0)
            glUniform1f(implementation_->pgxLocTime, time);
        if (implementation_->pgxLocResolution >= 0)
            glUniform2f(implementation_->pgxLocResolution,
                        static_cast<float>(width), static_cast<float>(height));
        if (implementation_->pgxLocDecay >= 0)
            glUniform1f(implementation_->pgxLocDecay,
                        implementation_->pgxFeedbackPrimed ? smoothed.feedbackDecay : 0.0f);
        if (implementation_->pgxLocZoom >= 0)
            glUniform1f(implementation_->pgxLocZoom, smoothed.feedbackZoom);
        if (implementation_->pgxLocRotation >= 0)
            glUniform1f(implementation_->pgxLocRotation, smoothed.feedbackRotation);
        if (implementation_->pgxLocWarp >= 0)
            glUniform1f(implementation_->pgxLocWarp, smoothed.feedbackWarp);
        if (implementation_->pgxLocBeatWarp >= 0)
            glUniform1f(implementation_->pgxLocBeatWarp, smoothed.feedbackBeatWarp);
        if (implementation_->pgxLocBeat >= 0)
            glUniform1f(implementation_->pgxLocBeat, smoothed.beat);
        if (implementation_->pgxLocEnergy >= 0)
            glUniform1f(implementation_->pgxLocEnergy, smoothed.energy);

        glBindVertexArray(fullscreenVAO_);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glBindVertexArray(0u);

        if (implementation_->pgxAudioShapeProgram != 0) {
            glEnable(GL_BLEND);
            glBlendFunc(GL_ONE, GL_ONE);
            glUseProgram(implementation_->pgxAudioShapeProgram);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, implementation_->pgxSpectrumTex);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, implementation_->pgxWaveformTex);
            if (implementation_->pgxShapeLocSpectrum >= 0)
                glUniform1i(implementation_->pgxShapeLocSpectrum, 0);
            if (implementation_->pgxShapeLocWaveform >= 0)
                glUniform1i(implementation_->pgxShapeLocWaveform, 1);
            if (implementation_->pgxShapeLocTime >= 0)
                glUniform1f(implementation_->pgxShapeLocTime, time);
            if (implementation_->pgxShapeLocResolution >= 0)
                glUniform2f(implementation_->pgxShapeLocResolution,
                            static_cast<float>(width), static_cast<float>(height));
            if (implementation_->pgxShapeLocWaveformMode >= 0)
                glUniform1i(implementation_->pgxShapeLocWaveformMode, smoothed.waveformMode);
            if (implementation_->pgxShapeLocWaveformOpacity >= 0)
                glUniform1f(implementation_->pgxShapeLocWaveformOpacity, smoothed.waveformOpacity);
            if (implementation_->pgxShapeLocWaveformThickness >= 0)
                glUniform1f(implementation_->pgxShapeLocWaveformThickness, smoothed.waveformThickness);
            if (implementation_->pgxShapeLocWaveformRadius >= 0)
                glUniform1f(implementation_->pgxShapeLocWaveformRadius, smoothed.waveformRadius);
            if (implementation_->pgxShapeLocWaveformBassResponse >= 0)
                glUniform1f(implementation_->pgxShapeLocWaveformBassResponse, smoothed.waveformBassResponse);
            if (implementation_->pgxShapeLocWaveformTrebleResponse >= 0)
                glUniform1f(implementation_->pgxShapeLocWaveformTrebleResponse, smoothed.waveformTrebleResponse);
            if (implementation_->pgxShapeLocBass >= 0)
                glUniform1f(implementation_->pgxShapeLocBass, smoothed.bass);
            if (implementation_->pgxShapeLocMid >= 0)
                glUniform1f(implementation_->pgxShapeLocMid, smoothed.mid);
            if (implementation_->pgxShapeLocTreble >= 0)
                glUniform1f(implementation_->pgxShapeLocTreble, smoothed.treble);
            if (implementation_->pgxShapeLocBeat >= 0)
                glUniform1f(implementation_->pgxShapeLocBeat, smoothed.beat);
            if (implementation_->pgxShapeLocEnergy >= 0)
                glUniform1f(implementation_->pgxShapeLocEnergy, smoothed.energy);
            if (implementation_->pgxShapeLocPhase >= 0)
                glUniform1f(implementation_->pgxShapeLocPhase, implementation_->pgxPhase);
            if (implementation_->pgxShapeLocSeed >= 0)
                glUniform1f(implementation_->pgxShapeLocSeed, implementation_->pgxSeed);
            if (implementation_->pgxShapeLocPrimary >= 0)
                glUniform3f(implementation_->pgxShapeLocPrimary,
                            smoothed.primaryColor.r, smoothed.primaryColor.g, smoothed.primaryColor.b);
            if (implementation_->pgxShapeLocSecondary >= 0)
                glUniform3f(implementation_->pgxShapeLocSecondary,
                            smoothed.secondaryColor.r, smoothed.secondaryColor.g, smoothed.secondaryColor.b);
            if (implementation_->pgxShapeLocAccent >= 0)
                glUniform3f(implementation_->pgxShapeLocAccent,
                            smoothed.accentColor.r, smoothed.accentColor.g, smoothed.accentColor.b);
            glBindVertexArray(fullscreenVAO_);
            glDrawArrays(GL_TRIANGLES, 0, 3);
            glBindVertexArray(0u);
            glDisable(GL_BLEND);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, 0);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, 0);
        }

        glBindFramebuffer(GL_READ_FRAMEBUFFER, implementation_->pgxFeedbackFBO[writeIndex]);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glBlitFramebuffer(0, 0, width, height, 0, 0, width, height,
                          GL_COLOR_BUFFER_BIT, GL_NEAREST);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, 0);

        implementation_->pgxReadIndex = writeIndex;
        implementation_->pgxFeedbackPrimed = true;
    } else {
        glBindFramebuffer(GL_READ_FRAMEBUFFER, implementation_->pgxSceneFBO);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glBlitFramebuffer(0, 0, width, height, 0, 0, width, height,
                          GL_COLOR_BUFFER_BIT, GL_NEAREST);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    // ── Debug overlay (rendered on top, uses its own program internally) ──
    if (implementation_->showDebugOverlay) {
        DebugState stateWithFps = debugState;
        stateWithFps.fps             = static_cast<float>(implementation_->currentFps);
        stateWithFps.rendererBackend = BackendName();
        stateWithFps.windowWidth     = width;
        stateWithFps.windowHeight    = height;
        stateWithFps.currentShader   = implementation_->shaderNames[
                                          static_cast<std::size_t>(implementation_->activeShader)].c_str();
        implementation_->debugOverlay.Render(stateWithFps, width, height);
    }

    // Brief shader-name toast after a switch (shown even with the overlay off).
    if (currentTime < implementation_->toastUntil) {
        implementation_->debugOverlay.RenderToast(
            implementation_->toastText.c_str(), width, height);
    }

    // Soundcheck input-level meter.
    if (implementation_->showMeter) {
        implementation_->debugOverlay.RenderMeter(debugState, width, height);
    }

    // Live-controls menu: sticky while toggled on (H), or auto-shown at startup
    // for a few seconds (fading out over the last second).
    float menuAlpha = 0.0f;
    if (implementation_->showMenu) {
        menuAlpha = 1.0f;
    } else if (currentTime < implementation_->menuVisibleUntil) {
        menuAlpha = std::clamp(
            static_cast<float>(implementation_->menuVisibleUntil - currentTime), 0.0f, 1.0f);
    }
    if (menuAlpha > 0.0f) {
        implementation_->debugOverlay.RenderMenu(width, height, menuAlpha);
    }

    // "BADMAN EXPERIENCE 4.0" brand banner (toggled by B key).
    if (implementation_->showTranceWordmark) {
        implementation_->debugOverlay.RenderTranceWordmark(
            width, height,
            static_cast<float>(currentTime - implementation_->tranceWordmarkStart), 1.0f);
    }
    if (implementation_->showBadmanWordmark) {
        implementation_->debugOverlay.RenderBadmanWordmark(
            width, height,
            static_cast<float>(currentTime - implementation_->badmanWordmarkStart), 1.0f);
    }
}

// ──────────────────────────────────────────────────────────────────────────────
// EndFrame
// ──────────────────────────────────────────────────────────────────────────────
bool OpenGLRenderer::EndFrame() {
    if (!initialized_) {
        return false;
    }

    glfwSwapBuffers(implementation_->window);
    glfwPollEvents();

    // ── Live controls (Task 7) ──────────────────────────────────────────────────
    // F1..F7 request theme slots 0..6.  Latch the rising edge here (events were
    // just polled) and let the Runtime drain it via ConsumeThemeRequest.
    constexpr int kThemeKeys[Implementation::kThemeKeyCount] = {
        GLFW_KEY_F1, GLFW_KEY_F2, GLFW_KEY_F3, GLFW_KEY_F4,
        GLFW_KEY_F5, GLFW_KEY_F6, GLFW_KEY_F7,
    };
    for (int i = 0; i < Implementation::kThemeKeyCount; ++i) {
        const bool pressed =
            glfwGetKey(implementation_->window, kThemeKeys[i]) == GLFW_PRESS;
        if (pressed && !implementation_->themeKeyWasPressed[i]) {
            implementation_->pendingThemeRequest = i;
        }
        implementation_->themeKeyWasPressed[i] = pressed;
    }

    // 'T' selects the opening TRANCE scene and wordmark.
    const bool tranceIsPressed =
        glfwGetKey(implementation_->window, GLFW_KEY_T) == GLFW_PRESS;
    if (tranceIsPressed && !implementation_->tranceKeyWasPressed) {
        implementation_->pendingTranceRequest = true;
        implementation_->showTranceWordmark = true;
        implementation_->showBadmanWordmark = false;
        implementation_->tranceWordmarkStart = glfwGetTime();
        for (std::size_t i = 0; i < implementation_->shaderNames.size(); ++i) {
            std::string name = implementation_->shaderNames[i];
            std::transform(name.begin(), name.end(), name.begin(),
                [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
            if (name.find("moonlit velvet") != std::string::npos) {
                BeginShaderTransition(static_cast<int>(i));
                break;
            }
        }
    }
    implementation_->tranceKeyWasPressed = tranceIsPressed;

    // 'B' selects the lively Badman/Amapiano section.
    const bool badmanIsPressed =
        glfwGetKey(implementation_->window, GLFW_KEY_B) == GLFW_PRESS;
    if (badmanIsPressed && !implementation_->badmanKeyWasPressed) {
        implementation_->pendingBadmanRequest = true;
        implementation_->showBadmanWordmark = true;
        implementation_->showTranceWordmark = false;
        implementation_->badmanWordmarkStart = glfwGetTime();
        int target = -1;
        for (std::size_t i = 0; i < implementation_->shaderNames.size(); ++i) {
            std::string name = implementation_->shaderNames[i];
            std::transform(name.begin(), name.end(), name.begin(),
                [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
            if (name.find("blood pulse") != std::string::npos) {
                target = static_cast<int>(i);
                break;
            }
            if (target < 0 && name.find("inferno lasers") != std::string::npos) {
                target = static_cast<int>(i);
            }
        }
        if (target >= 0) BeginShaderTransition(target);
    }
    implementation_->badmanKeyWasPressed = badmanIsPressed;

    const bool inputIsPressed =
        glfwGetKey(implementation_->window, GLFW_KEY_I) == GLFW_PRESS;
    if (inputIsPressed && !implementation_->inputKeyWasPressed) {
        implementation_->pendingInputSwitch = true;
    }
    implementation_->inputKeyWasPressed = inputIsPressed;

    // Space toggles audio play/pause.
    const bool spaceIsPressed =
        glfwGetKey(implementation_->window, GLFW_KEY_SPACE) == GLFW_PRESS;
    if (spaceIsPressed && !implementation_->spaceWasPressed) {
        implementation_->pendingPlayPause = true;
    }
    implementation_->spaceWasPressed = spaceIsPressed;

    // F8 reloads the current theme's JSON from disk (no restart).
    const bool reloadIsPressed =
        glfwGetKey(implementation_->window, GLFW_KEY_F8) == GLFW_PRESS;
    if (reloadIsPressed && !implementation_->reloadKeyWasPressed) {
        implementation_->pendingReload = true;
    }
    implementation_->reloadKeyWasPressed = reloadIsPressed;

    // 'A' toggles Auto-VJ (automatic preset/form selection).
    const bool autoKeyIsPressed =
        glfwGetKey(implementation_->window, GLFW_KEY_A) == GLFW_PRESS;
    if (autoKeyIsPressed && !implementation_->autoKeyWasPressed) {
        implementation_->pendingAutoToggle = true;
    }
    implementation_->autoKeyWasPressed = autoKeyIsPressed;

    // F9 toggles demo presentation mode (fullscreen + cursor hide + no overlay).
    const bool demoKeyIsPressed =
        glfwGetKey(implementation_->window, GLFW_KEY_F9) == GLFW_PRESS;
    if (demoKeyIsPressed && !implementation_->demoKeyWasPressed) {
        SetDemoMode(!implementation_->demoMode);
    }
    implementation_->demoKeyWasPressed = demoKeyIsPressed;

    // F10 toggles visual FX (glow / bloom / noise).
    const bool fxKeyIsPressed =
        glfwGetKey(implementation_->window, GLFW_KEY_F10) == GLFW_PRESS;
    if (fxKeyIsPressed && !implementation_->fxKeyWasPressed) {
        implementation_->fxEnabled = !implementation_->fxEnabled;
    }
    implementation_->fxKeyWasPressed = fxKeyIsPressed;

    // F11 toggles fullscreen.
    const bool fsKeyIsPressed =
        glfwGetKey(implementation_->window, GLFW_KEY_F11) == GLFW_PRESS;
    if (fsKeyIsPressed && !implementation_->fullscreenKeyWasPressed) {
        SetFullscreen(!implementation_->isFullscreen);
    }
    implementation_->fullscreenKeyWasPressed = fsKeyIsPressed;

    // '[' / ']' cycle the active shader through the library (signature + pack).
    const int shaderCount = 1 + static_cast<int>(implementation_->shaderLib.size());
    const auto switchShader = [&](int delta) {
        BeginShaderTransition((implementation_->activeShader + delta + shaderCount) % shaderCount);
    };
    const bool prevSh = glfwGetKey(implementation_->window, GLFW_KEY_LEFT_BRACKET) == GLFW_PRESS;
    if (prevSh && !implementation_->prevShaderKeyPressed && shaderCount > 1) switchShader(-1);
    implementation_->prevShaderKeyPressed = prevSh;
    const bool nextSh = glfwGetKey(implementation_->window, GLFW_KEY_RIGHT_BRACKET) == GLFW_PRESS;
    if (nextSh && !implementation_->nextShaderKeyPressed && shaderCount > 1) switchShader(1);
    implementation_->nextShaderKeyPressed = nextSh;

    // 'V' toggles the music-driven auto-shader director.
    const bool vKey = glfwGetKey(implementation_->window, GLFW_KEY_V) == GLFW_PRESS;
    if (vKey && !implementation_->autoShaderKeyWasPressed) {
        implementation_->autoShader = !implementation_->autoShader;
        implementation_->lastSwitchTime = glfwGetTime();
        implementation_->toastText  = implementation_->autoShader ? "Auto-shader: ON" : "Auto-shader: OFF";
        implementation_->toastUntil = glfwGetTime() + 2.0;
    }
    implementation_->autoShaderKeyWasPressed = vKey;

    // 'M' toggles the soundcheck input-level meter.
    const bool meterKeyIsPressed =
        glfwGetKey(implementation_->window, GLFW_KEY_M) == GLFW_PRESS;
    if (meterKeyIsPressed && !implementation_->meterKeyWasPressed) {
        implementation_->showMeter = !implementation_->showMeter;
    }
    implementation_->meterKeyWasPressed = meterKeyIsPressed;

    // 'H' toggles the live-controls menu (all keys + features).
    const bool menuKeyIsPressed =
        glfwGetKey(implementation_->window, GLFW_KEY_H) == GLFW_PRESS;
    if (menuKeyIsPressed && !implementation_->menuKeyWasPressed) {
        implementation_->showMenu = !implementation_->showMenu;
        implementation_->menuVisibleUntil = 0.0; // cancel any startup auto-show
    }
    implementation_->menuKeyWasPressed = menuKeyIsPressed;

    // F12 toggles the debug overlay.
    const bool debugKeyIsPressed =
        glfwGetKey(implementation_->window, GLFW_KEY_F12) == GLFW_PRESS;
    if (debugKeyIsPressed && !implementation_->debugKeyWasPressed) {
        implementation_->showDebugOverlay = !implementation_->showDebugOverlay;
    }
    implementation_->debugKeyWasPressed = debugKeyIsPressed;

    // ESC requests application exit (EndFrame then reports the close).
    if (glfwGetKey(implementation_->window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(implementation_->window, GLFW_TRUE);
    }

    // Cursor auto-hide: hide after a few idle seconds in demo/fullscreen, and
    // restore the moment the mouse moves.
    {
        double cx = 0.0;
        double cy = 0.0;
        glfwGetCursorPos(implementation_->window, &cx, &cy);
        const double now = glfwGetTime();
        const double dx = cx - implementation_->lastCursorX;
        const double dy = cy - implementation_->lastCursorY;
        if (dx * dx + dy * dy > 4.0) { // moved more than ~2 px
            implementation_->lastActivityTime = now;
            implementation_->lastCursorX = cx;
            implementation_->lastCursorY = cy;
            if (implementation_->cursorHidden) {
                glfwSetInputMode(implementation_->window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                implementation_->cursorHidden = false;
            }
        } else if (!implementation_->cursorHidden &&
                   (implementation_->demoMode || implementation_->isFullscreen) &&
                   now - implementation_->lastActivityTime > 3.0) {
            glfwSetInputMode(implementation_->window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
            implementation_->cursorHidden = true;
        }
    }

    // FPS counter and window title update.
    ++implementation_->renderedFrameCount;
    const double currentTime  = glfwGetTime();
    const double elapsedTime  = currentTime - implementation_->lastFpsUpdateTime;
    if (elapsedTime >= 1.0) {
        implementation_->currentFps =
            static_cast<double>(implementation_->renderedFrameCount) / elapsedTime;

        std::ostringstream title;
        title << "PAPAGEDON Core v0.0.1 | FPS: "
              << std::lround(implementation_->currentFps);
        glfwSetWindowTitle(implementation_->window, title.str().c_str());

        implementation_->renderedFrameCount = 0;
        implementation_->lastFpsUpdateTime  = currentTime;
    }

    return glfwWindowShouldClose(implementation_->window) == GLFW_FALSE;
}

// ──────────────────────────────────────────────────────────────────────────────
// ConsumePresetRequest
// ──────────────────────────────────────────────────────────────────────────────
int OpenGLRenderer::ConsumeThemeRequest() noexcept {
    if (implementation_ == nullptr) {
        return -1;
    }
    const int request = implementation_->pendingThemeRequest;
    implementation_->pendingThemeRequest = -1;
    return request;
}

bool OpenGLRenderer::ConsumeTranceRequest() noexcept {
    if (implementation_ == nullptr) {
        return false;
    }
    const bool requested = implementation_->pendingTranceRequest;
    implementation_->pendingTranceRequest = false;
    return requested;
}

bool OpenGLRenderer::ConsumeBadmanRequest() noexcept {
    if (!implementation_) {
        return false;
    }
    const bool requested = implementation_->pendingBadmanRequest;
    implementation_->pendingBadmanRequest = false;
    return requested;
}

bool OpenGLRenderer::ConsumeInputSwitchRequest() noexcept {
    if (!implementation_) {
        return false;
    }
    const bool requested = implementation_->pendingInputSwitch;
    implementation_->pendingInputSwitch = false;
    return requested;
}

bool OpenGLRenderer::ConsumePlayPauseToggle() noexcept {
    if (implementation_ == nullptr) {
        return false;
    }
    const bool toggled = implementation_->pendingPlayPause;
    implementation_->pendingPlayPause = false;
    return toggled;
}

bool OpenGLRenderer::ConsumeReloadRequest() noexcept {
    if (implementation_ == nullptr) {
        return false;
    }
    const bool requested = implementation_->pendingReload;
    implementation_->pendingReload = false;
    return requested;
}

bool OpenGLRenderer::ConsumeAutoToggle() noexcept {
    if (implementation_ == nullptr) {
        return false;
    }
    const bool toggled = implementation_->pendingAutoToggle;
    implementation_->pendingAutoToggle = false;
    return toggled;
}

std::string OpenGLRenderer::ConsumeDroppedFile() noexcept {
    if (implementation_ == nullptr || !implementation_->pendingDroppedFile) {
        return {};
    }
    std::string path = std::move(implementation_->droppedFilePath);
    implementation_->droppedFilePath.clear();
    implementation_->pendingDroppedFile = false;
    return path;
}

// ──────────────────────────────────────────────────────────────────────────────
// Shutdown
// ──────────────────────────────────────────────────────────────────────────────
void OpenGLRenderer::Shutdown() noexcept {
    if (implementation_ == nullptr || !initialized_) {
        return;
    }

    implementation_->debugOverlay.Shutdown();

    shaderManager_.Shutdown();
    for (auto& sm : implementation_->shaderLib) {
        if (sm) sm->Shutdown();
    }
    implementation_->shaderLib.clear();
    if (implementation_->transitionProgram != 0) {
        glDeleteProgram(implementation_->transitionProgram);
        implementation_->transitionProgram = 0;
    }
    glDeleteFramebuffers(2, implementation_->transitionFBO);
    implementation_->transitionFBO[0] = implementation_->transitionFBO[1] = 0;
    glDeleteTextures(2, implementation_->transitionTex);
    implementation_->transitionTex[0] = implementation_->transitionTex[1] = 0;
    if (implementation_->pgxFeedbackProgram != 0) {
        glDeleteProgram(implementation_->pgxFeedbackProgram);
        implementation_->pgxFeedbackProgram = 0;
    }
    if (implementation_->pgxAudioShapeProgram != 0) {
        glDeleteProgram(implementation_->pgxAudioShapeProgram);
        implementation_->pgxAudioShapeProgram = 0;
    }
    glDeleteTextures(1, &implementation_->pgxSpectrumTex);
    implementation_->pgxSpectrumTex = 0;
    glDeleteTextures(1, &implementation_->pgxWaveformTex);
    implementation_->pgxWaveformTex = 0;
    glDeleteFramebuffers(1, &implementation_->pgxSceneFBO);
    implementation_->pgxSceneFBO = 0;
    glDeleteTextures(1, &implementation_->pgxSceneTex);
    implementation_->pgxSceneTex = 0;
    glDeleteFramebuffers(2, implementation_->pgxFeedbackFBO);
    implementation_->pgxFeedbackFBO[0] = implementation_->pgxFeedbackFBO[1] = 0;
    glDeleteTextures(2, implementation_->pgxFeedbackTex);
    implementation_->pgxFeedbackTex[0] = implementation_->pgxFeedbackTex[1] = 0;
    if (implementation_->introProgram != 0) {
        glDeleteProgram(implementation_->introProgram);
        implementation_->introProgram = 0;
    }

    if (fullscreenVAO_ != 0u) {
        glDeleteVertexArrays(1, &fullscreenVAO_);
        fullscreenVAO_ = 0u;
    }

    glfwDestroyWindow(implementation_->window);
    implementation_->window = nullptr;
    glfwTerminate();
    initialized_ = false;
}

} // namespace papagedon
