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
// Uniform mapping (per Stage 5.1 spec):
//   uEnergy    → HSV value  (brightness)
//   uIntensity → HSV saturation
//   uBass      → ring pulse radius / speed
//   uMid       → spiral arm density
//   uTreble    → fine-detail ripple amplitude
//   uBeat      → instantaneous flash at the centre
//   uTime      → animation clock (seconds)
//   uResolution→ viewport size for aspect correction
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

// ── Utility ──────────────────────────────────────────────────────────────────

vec3 hsv2rgb(vec3 c) {
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

// Smooth modulo for seamless tiling
float smod(float x, float m) { return x - m * floor(x / m); }

// ── Main ─────────────────────────────────────────────────────────────────────
void main() {
    const float kPi  = 3.14159265358979;
    const float kTau = 6.28318530717959;

    // Aspect-correct UV, centred at (0,0)
    vec2 uv = (vUV * 2.0 - 1.0) * vec2(uResolution.x / uResolution.y, 1.0);

    float t     = uTime * 0.25;
    float dist  = length(uv);
    float angle = atan(uv.y, uv.x);   // [-π, π]

    // ── Layer 1: Bass-driven concentric rings ─────────────────────────────
    float ringFreq  = 7.0 + uBass * 14.0;
    float ringSpeed = 1.5 + uBass * 3.0;
    float rings     = sin(dist * ringFreq - t * ringSpeed) * 0.5 + 0.5;
    // Pulse the ring amplitude on beat
    rings += uBeat * 0.35 * exp(-dist * 3.0);

    // ── Layer 2: Mid-driven rotating spiral ───────────────────────────────
    float spiralArms  = 3.0 + uMid * 4.0;
    float spiralPhase = angle / kTau + dist * 2.5 - t * 0.8;
    float spiral      = sin(spiralPhase * spiralArms * kTau) * 0.5 + 0.5;

    // ── Layer 3: Treble-driven high-frequency shimmer ─────────────────────
    float shimmerX  = sin(uv.x * 22.0 + t * 1.7) * sin(uv.y * 22.0 - t * 1.3);
    float shimmerY  = cos(uv.x * 15.0 - t * 2.1) * cos(uv.y * 15.0 + t * 0.9);
    float shimmer   = (shimmerX + shimmerY) * 0.5 * uTreble;

    // ── Combine layers ────────────────────────────────────────────────────
    float pattern = rings * 0.50
                  + spiral * 0.35
                  + shimmer * 0.15;
    pattern = clamp(pattern, 0.0, 1.0);

    // ── Colour ────────────────────────────────────────────────────────────
    // Hue: slowly rotates with time; bass nudges it for big impacts.
    float hue = fract(t * 0.04 + uBass * 0.25 - pattern * 0.12);

    // Energy drives brightness; intensity drives saturation.
    float brightness  = 0.07 + uEnergy * 0.88;
    float saturation  = 0.10 + uIntensity * 0.90;

    vec3 color = hsv2rgb(vec3(hue, saturation, brightness * pattern));

    // ── Beat centre flash ─────────────────────────────────────────────────
    // A radial white burst on the beat frame, decaying with distance.
    float beatGlow = uBeat * 0.6 * exp(-dist * dist * 4.0);
    color += vec3(beatGlow);

    // ── Vignette ──────────────────────────────────────────────────────────
    float vignette = 1.0 - smoothstep(0.55, 1.45, dist);
    color *= vignette;

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
