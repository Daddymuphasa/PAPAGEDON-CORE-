#include "ShaderManager.h"

#include <glad/glad.h>

#include <array>
#include <cstdio>

namespace papagedon {

// ──────────────────────────────────────────────────────────────────────────────
// Embedded GLSL source
//
// Shaders are embedded as string literals to avoid working-directory ambiguity
// on Windows and to guarantee the shader is always present.  The ShaderManager
// interface supports any source string, so file-based loading (Stage 7 Asset
// System) can be dropped in without touching this class.
// ──────────────────────────────────────────────────────────────────────────────

// One oversized triangle covers the full viewport without any VBO data.
// gl_VertexID selects the clip-space corner from a hardcoded table.
static constexpr const char* kDefaultVertexSource = R"GLSL(
#version 460 core

out vec2 vUV;

void main() {
    const vec2 kPositions[3] = vec2[3](
        vec2(-1.0, -1.0),
        vec2( 3.0, -1.0),
        vec2(-1.0,  3.0)
    );
    vec2 pos   = kPositions[gl_VertexID];
    gl_Position = vec4(pos, 0.0, 1.0);
    vUV         = pos * 0.5 + 0.5;
}
)GLSL";

// ──────────────────────────────────────────────────────────────────────────────
// Reactive fragment shader
//
// Rather than one fixed composition, the shader carries six signature *forms* —
// one per preset — built from domain-warped fractal noise (fbm) so the motion is
// organic and evolving instead of a looping sine field.  uPattern selects the
// form; during a preset switch the Renderer cross-fades uPrevPattern → uPattern
// as uPatternBlend eases 0 → 1, so the whole look morphs rather than popping.
//
// Uniform mapping:
//   uEnergy    → overall brightness (already scaled by the preset on the CPU)
//   uIntensity → colour saturation input
//   uBass/uMid/uTreble → per-form reactive hooks (swell, speed, sparkle …)
//   uBeat      → beat pulse (peaks at the preset's beat response)
//   uMood      → warm/cool tint  (0 = cool, 1 = warm)
//   uColorLow/Mid/High → active preset palette (eased across switches on CPU)
//   uBackground        → preset ambient colour filling the darkest regions
//   uSaturationBase/Scale → preset saturation curve (base + intensity * scale)
//   uMotion    → preset animation-speed multiplier
//   uPattern / uPrevPattern / uPatternBlend → signature form + switch cross-fade
//   uWarp      → domain-warp strength (turbulence)
//   uDetail    → fractal detail emphasis (high-frequency retention)
//   uTime / uResolution → animation clock and aspect correction
// ──────────────────────────────────────────────────────────────────────────────
static constexpr const char* kDefaultFragmentSource = R"GLSL(
#version 460 core

in  vec2 vUV;
out vec4 fragColor;

uniform float uTime;
uniform vec2  uResolution;
uniform float uEnergy;
uniform float uIntensity;
uniform float uBass;
uniform float uMid;
uniform float uTreble;
uniform float uBeat;
uniform float uMood;
uniform vec3  uColorLow;
uniform vec3  uColorMid;
uniform vec3  uColorHigh;
uniform vec3  uBackground;
uniform float uSaturationBase;
uniform float uSaturationScale;
uniform float uMotion;
uniform int   uPattern;
uniform int   uPrevPattern;
uniform float uPatternBlend;
uniform float uWarp;
uniform float uDetail;

const float kPi  = 3.14159265358979;
const float kTau = 6.28318530717959;

// ── Palette ────────────────────────────────────────────────────────────────
// Three-stop palette ramp: low → mid → high across t in [0, 1].
vec3 palette(float t) {
    t = clamp(t, 0.0, 1.0);
    return t < 0.5
        ? mix(uColorLow, uColorMid,  t * 2.0)
        : mix(uColorMid, uColorHigh, (t - 0.5) * 2.0);
}

// ── Noise toolkit ────────────────────────────────────────────────────────────
float hash21(vec2 p) {
    p = fract(p * vec2(123.34, 345.45));
    p += dot(p, p + 34.345);
    return fract(p.x * p.y);
}

// Smooth value noise.
float vnoise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    vec2 u = f * f * (3.0 - 2.0 * f);
    float a = hash21(i + vec2(0.0, 0.0));
    float b = hash21(i + vec2(1.0, 0.0));
    float c = hash21(i + vec2(0.0, 1.0));
    float d = hash21(i + vec2(1.0, 1.0));
    return mix(mix(a, b, u.x), mix(c, d, u.x), u.y);
}

// Fractal brownian motion.  uDetail keeps more energy in the high octaves.
// Three octaves is enough once domain-warped — it hides the missing octave —
// and keeps the per-pixel noise cost within the 120 FPS frame budget.
float fbm(vec2 p) {
    float sum  = 0.0;
    float amp  = 0.5;
    float freq = 1.0;
    float gain = clamp(0.5 + 0.12 * (uDetail - 1.0), 0.38, 0.66);
    for (int i = 0; i < 3; ++i) {
        sum  += amp * vnoise(p * freq);
        freq *= 2.0;
        amp  *= gain;
    }
    return sum;
}

// Domain-warped fbm: fbm sampled through an fbm-displaced coordinate field.
float warpedFbm(vec2 p, float t) {
    vec2 q = vec2(fbm(p + vec2(0.0, t * 0.10)),
                  fbm(p + vec2(5.2, 1.3) - vec2(t * 0.08, 0.0)));
    return fbm(p + uWarp * q);
}

// ── Signature forms ──────────────────────────────────────────────────────────
// Each returns an intensity in [0, 1] that the palette then maps to colour.

// 0: Aurora — vertical flowing curtains that rise and ripple.
float patAurora(vec2 uv, float t) {
    float w       = warpedFbm(vec2(uv.x * 1.3, uv.y * 0.7 - t * 0.4), t);
    float curtain = warpedFbm(vec2(uv.x * 2.2 + w * 1.5, uv.y * 0.5 - t * 0.35), t * 1.3);
    float shape   = smoothstep(0.25, 0.9, curtain);
    float vfall   = smoothstep(1.2, -0.8, uv.y);   // brighter toward the bottom
    return clamp((shape + uBass * 0.25) * (0.35 + 0.9 * vfall), 0.0, 1.0);
}

// 1: Nebula — drifting volumetric clouds with treble sparkle.
float patNebula(vec2 uv, float t) {
    vec2  p      = uv * 1.1 + vec2(t * 0.05, -t * 0.04);
    float clouds = warpedFbm(p, t * 0.6);
    clouds       = pow(clamp(clouds * 1.4, 0.0, 1.0), 1.6);
    float star   = hash21(floor(uv * 60.0));
    float tw     = step(0.985 - uTreble * 0.03, star)
                 * (0.5 + 0.5 * sin(t * 8.0 + star * 40.0));
    return clamp(clouds + tw * uTreble, 0.0, 1.0);
}

// 2: Matrix — falling digital-rain columns.
float patMatrix(vec2 uv, float t) {
    float cols  = 40.0;
    float col   = floor((uv.x * 0.5 + 0.5) * cols);
    float y     = uv.y * 0.5 + 0.5;
    float speed = 0.25 + hash21(vec2(col, 3.0)) * (0.5 + uMid * 1.5);
    float head  = fract(t * speed + hash21(vec2(col, 7.0)));
    float d     = fract(head - y);
    float stream = pow(1.0 - d, 3.0);              // bright head, fading trail
    float cell  = hash21(vec2(col, floor(y * cols)));
    float flick = 0.55 + 0.45 * sin(t * 9.0 + cell * 33.0);
    return clamp(stream * flick * (0.7 + uEnergy * 0.6), 0.0, 1.0);
}

// 3: Liquid — smooth caustic ripples, like light on water.
float patLiquid(vec2 uv, float t) {
    vec2  p       = uv * 1.6;
    float a       = warpedFbm(p + vec2(t * 0.12, t * 0.09), t);
    float b       = warpedFbm(p * 1.3 - vec2(t * 0.08, t * 0.11), t * 0.8);
    float caustic = abs(a - b);                    // ridged → vein-like
    float liquid  = 1.0 - smoothstep(0.0, 0.35, caustic);
    return clamp(pow(liquid, 1.4) * (0.5 + 0.7 * a) + uBass * 0.15, 0.0, 1.0);
}

// 4: Tunnel — perspective tunnel rushing inward.
float patTunnel(vec2 uv, float t) {
    float r     = max(length(uv), 1e-3);
    float ang   = atan(uv.y, uv.x);
    float depth = 1.0 / r + t * (1.2 + uEnergy * 1.5) + uBeat * 0.6;
    float walls = warpedFbm(vec2(ang / kPi * 3.0, depth * 2.0), t);
    float rings = 0.5 + 0.5 * sin(depth * kTau * (1.0 + uMid));
    float g     = mix(walls, rings, 0.5);
    g          *= smoothstep(0.0, 0.35, r);        // dark centre (far away)
    return clamp(g * (0.5 + uEnergy * 0.8), 0.0, 1.0);
}

// 5: Pulse — kaleidoscopic radial shockwaves retriggered on beats.
float patPulse(vec2 uv, float t) {
    float ang  = atan(uv.y, uv.x);
    float wedge = abs(fract(ang / kTau * 8.0) - 0.5); // mirror into 8 wedges
    float r    = length(uv);
    float wave = 0.5 + 0.5 * sin(r * (14.0 + uBass * 20.0) - t * 6.0 - uBeat * 8.0);
    float burst = uBeat * exp(-r * 3.0) * 1.5;
    float tex  = warpedFbm(vec2(wedge * 6.0, r * 4.0 - t), t);
    float g    = wave * (0.4 + 0.6 * tex) + burst;
    g         *= smoothstep(1.4, 0.1, r);          // fade toward the edges
    return clamp(g, 0.0, 1.0);
}

float patternFor(int mode, vec2 uv, float t) {
    if (mode == 0) return patAurora(uv, t);
    if (mode == 1) return patNebula(uv, t);
    if (mode == 2) return patMatrix(uv, t);
    if (mode == 3) return patLiquid(uv, t);
    if (mode == 4) return patTunnel(uv, t);
    return patPulse(uv, t);
}

// ── Main ─────────────────────────────────────────────────────────────────────
void main() {
    // Aspect-correct UV, centred at (0,0).
    vec2 uv = (vUV * 2.0 - 1.0) * vec2(uResolution.x / uResolution.y, 1.0);

    // Motion multiplier lets each preset run languid or frantic on the same clock.
    float t = uTime * 0.25 * uMotion;

    // Active form, cross-fading from the outgoing form during a preset switch.
    float pattern = patternFor(uPattern, uv, t);
    if (uPatternBlend < 0.999 && uPrevPattern != uPattern) {
        float prev = patternFor(uPrevPattern, uv, t);
        pattern = mix(prev, pattern, uPatternBlend);
    }
    pattern = clamp(pattern, 0.0, 1.0);

    // ── Colour ────────────────────────────────────────────────────────────
    // The active preset's palette maps across the pattern intensity.  Bass nudges
    // the ramp lookup so heavy low-end pushes toward the palette's bright stop.
    vec3 color = palette(pattern + uBass * 0.12);

    // Preset saturation curve: intensity fades toward luma per the preset's shape.
    float luma = dot(color, vec3(0.299, 0.587, 0.114));
    float saturation = clamp(uSaturationBase + uIntensity * uSaturationScale, 0.0, 1.0);
    color = mix(vec3(luma), color, saturation);

    // Energy drives overall brightness, shaped by the pattern.
    float brightness = 0.08 + uEnergy * 0.9;
    color *= brightness * (0.35 + 0.85 * pattern);

    // The preset's ambient background fills the darkest regions so each preset
    // keeps a distinct base tone even where the pattern falls to zero.
    color += uBackground * (1.0 - pattern);

    // Mood tints warm (>0.5) or cool (<0.5) without leaving the palette behind.
    vec3 warmTint = vec3(1.12, 1.0, 0.85);
    vec3 coolTint = vec3(0.85, 1.0, 1.12);
    color *= mix(coolTint, warmTint, clamp(uMood, 0.0, 1.0));

    // Beat bloom tinted by the palette's brightest stop, shaped by the pattern.
    color += mix(vec3(1.0), uColorHigh, 0.5) * uBeat * 0.25 * pattern;

    // Subtle vignette.
    float d = length(uv);
    color *= 1.0 - smoothstep(0.7, 1.6, d);

    fragColor = vec4(clamp(color, 0.0, 1.0), 1.0);
}
)GLSL";

// ──────────────────────────────────────────────────────────────────────────────
// Internal helpers
// ──────────────────────────────────────────────────────────────────────────────
namespace {

[[nodiscard]] unsigned int CompileStage(
    const GLenum type,
    const char* const source) noexcept {

    const unsigned int shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    int ok = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (ok == 0) {
        std::array<char, 1024> log{};
        glGetShaderInfoLog(shader, static_cast<int>(log.size()), nullptr, log.data());
        std::fprintf(stderr,
            "[ShaderManager] GLSL compile error (%s):\n%s\n",
            type == GL_VERTEX_SHADER ? "vertex" : "fragment",
            log.data());
        glDeleteShader(shader);
        return 0u;
    }
    return shader;
}

} // namespace

// ──────────────────────────────────────────────────────────────────────────────
// ShaderManager — public API
// ──────────────────────────────────────────────────────────────────────────────

ShaderManager::~ShaderManager() {
    Shutdown();
}

bool ShaderManager::Compile(
    const char* const vertexSource,
    const char* const fragmentSource) noexcept {

    Shutdown(); // release any previous program

    const unsigned int vert = CompileStage(GL_VERTEX_SHADER,   vertexSource);
    if (vert == 0u) return false;

    const unsigned int frag = CompileStage(GL_FRAGMENT_SHADER, fragmentSource);
    if (frag == 0u) {
        glDeleteShader(vert);
        return false;
    }

    const unsigned int prog = glCreateProgram();
    glAttachShader(prog, vert);
    glAttachShader(prog, frag);
    glLinkProgram(prog);

    // Shaders are fully baked into the program; no longer needed.
    glDetachShader(prog, vert);
    glDetachShader(prog, frag);
    glDeleteShader(vert);
    glDeleteShader(frag);

    int ok = 0;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (ok == 0) {
        std::array<char, 1024> log{};
        glGetProgramInfoLog(prog, static_cast<int>(log.size()), nullptr, log.data());
        std::fprintf(stderr, "[ShaderManager] GLSL link error:\n%s\n", log.data());
        glDeleteProgram(prog);
        return false;
    }

    program_ = prog;

    // Cache uniform locations — resolved once, used every frame.
    locTime_       = glGetUniformLocation(program_, "uTime");
    locResolution_ = glGetUniformLocation(program_, "uResolution");
    locEnergy_     = glGetUniformLocation(program_, "uEnergy");
    locIntensity_  = glGetUniformLocation(program_, "uIntensity");
    locBass_       = glGetUniformLocation(program_, "uBass");
    locMid_        = glGetUniformLocation(program_, "uMid");
    locTreble_     = glGetUniformLocation(program_, "uTreble");
    locBeat_       = glGetUniformLocation(program_, "uBeat");
    locMood_       = glGetUniformLocation(program_, "uMood");
    locColorLow_   = glGetUniformLocation(program_, "uColorLow");
    locColorMid_   = glGetUniformLocation(program_, "uColorMid");
    locColorHigh_  = glGetUniformLocation(program_, "uColorHigh");
    locBackground_      = glGetUniformLocation(program_, "uBackground");
    locSaturationBase_  = glGetUniformLocation(program_, "uSaturationBase");
    locSaturationScale_ = glGetUniformLocation(program_, "uSaturationScale");
    locMotion_          = glGetUniformLocation(program_, "uMotion");
    locPattern_         = glGetUniformLocation(program_, "uPattern");
    locPrevPattern_     = glGetUniformLocation(program_, "uPrevPattern");
    locPatternBlend_    = glGetUniformLocation(program_, "uPatternBlend");
    locWarp_            = glGetUniformLocation(program_, "uWarp");
    locDetail_          = glGetUniformLocation(program_, "uDetail");

    return true;
}

void ShaderManager::Bind() const noexcept {
    if (program_ != 0u) {
        glUseProgram(program_);
    }
}

void ShaderManager::SetUniforms(
    const ShaderUniforms& u,
    const float time,
    const int width,
    const int height) const noexcept {

    if (program_ == 0u) {
        return;
    }

    // glUniform* calls use pre-cached locations — no hash lookup per frame.
    if (locTime_       >= 0) glUniform1f(locTime_,       time);
    if (locResolution_ >= 0) glUniform2f(locResolution_, static_cast<float>(width),
                                                         static_cast<float>(height));
    if (locEnergy_     >= 0) glUniform1f(locEnergy_,     u.energy);
    if (locIntensity_  >= 0) glUniform1f(locIntensity_,  u.intensity);
    if (locBass_       >= 0) glUniform1f(locBass_,        u.bass);
    if (locMid_        >= 0) glUniform1f(locMid_,         u.mid);
    if (locTreble_     >= 0) glUniform1f(locTreble_,      u.treble);
    if (locBeat_       >= 0) glUniform1f(locBeat_,        u.beat);
    if (locMood_       >= 0) glUniform1f(locMood_,        u.mood);
    if (locColorLow_   >= 0) glUniform3f(locColorLow_,    u.colorLow.r,  u.colorLow.g,  u.colorLow.b);
    if (locColorMid_   >= 0) glUniform3f(locColorMid_,    u.colorMid.r,  u.colorMid.g,  u.colorMid.b);
    if (locColorHigh_  >= 0) glUniform3f(locColorHigh_,   u.colorHigh.r, u.colorHigh.g, u.colorHigh.b);
    if (locBackground_      >= 0) glUniform3f(locBackground_, u.background.r, u.background.g, u.background.b);
    if (locSaturationBase_  >= 0) glUniform1f(locSaturationBase_,  u.saturationBase);
    if (locSaturationScale_ >= 0) glUniform1f(locSaturationScale_, u.saturationScale);
    if (locMotion_          >= 0) glUniform1f(locMotion_,          u.motion);
    if (locPattern_         >= 0) glUniform1i(locPattern_,         u.patternMode);
    if (locPrevPattern_     >= 0) glUniform1i(locPrevPattern_,     u.previousPatternMode);
    if (locPatternBlend_    >= 0) glUniform1f(locPatternBlend_,    u.patternBlend);
    if (locWarp_            >= 0) glUniform1f(locWarp_,            u.warp);
    if (locDetail_          >= 0) glUniform1f(locDetail_,          u.detail);
}

void ShaderManager::Shutdown() noexcept {
    if (program_ != 0u) {
        glDeleteProgram(program_);
        program_      = 0u;
        locTime_      = -1;
        locResolution_ = -1;
        locEnergy_    = -1;
        locIntensity_ = -1;
        locBass_      = -1;
        locMid_       = -1;
        locTreble_    = -1;
        locBeat_      = -1;
        locMood_      = -1;
        locColorLow_  = -1;
        locColorMid_  = -1;
        locColorHigh_ = -1;
        locBackground_      = -1;
        locSaturationBase_  = -1;
        locSaturationScale_ = -1;
        locMotion_          = -1;
        locPattern_         = -1;
        locPrevPattern_     = -1;
        locPatternBlend_    = -1;
        locWarp_            = -1;
        locDetail_          = -1;
    }
}

// ──────────────────────────────────────────────────────────────────────────────
// Default shader source accessors
// ──────────────────────────────────────────────────────────────────────────────

const char* ShaderManager::DefaultVertexSource() noexcept {
    return kDefaultVertexSource;
}

const char* ShaderManager::DefaultFragmentSource() noexcept {
    return kDefaultFragmentSource;
}

} // namespace papagedon
