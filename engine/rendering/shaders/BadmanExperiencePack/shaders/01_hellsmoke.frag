#version 460 core
// ─────────────────────────────────────────────────────────────────────────────
//  01 · HELLSMOKE  —  Badman Experience Pack Vol.1
//  Large volumetric smoke lit from within. Bass expands the plume, beats fire
//  radial pressure waves, treble scatters glowing embers. Slow, heavy, cinematic.
//  PAPAGEDON · dark-luxury rave visual system · designed for LED walls
// ─────────────────────────────────────────────────────────────────────────────
in  vec2 vUV;
out vec4 fragColor;

uniform float uTime;
uniform vec2  uResolution;
uniform float uBass;
uniform float uMid;
uniform float uTreble;
uniform float uBeat;
uniform float uEnergy;
uniform float uIntensity;
uniform vec3  uPrimaryColour;    // Badman red    #A30D18
uniform vec3  uSecondaryColour;  // near-black     #050505
uniform vec3  uAccentColour;     // gold           #D6A53A
uniform vec3  uBackground;       // shadow         #180202

const vec3 BAD_RED    = vec3(0.639, 0.051, 0.094);
const vec3 BAD_BLACK  = vec3(0.020, 0.020, 0.020);
const vec3 BAD_GOLD   = vec3(0.839, 0.647, 0.227);
const vec3 BAD_CREAM  = vec3(0.965, 0.945, 0.922);
const vec3 BAD_SHADOW = vec3(0.094, 0.008, 0.008);
const float TAU = 6.28318530718;

vec3  themed(vec3 t, vec3 f) { return dot(t, t) > 1e-4 ? t : f; }
float hash21(vec2 p) { p = fract(p * vec2(123.34, 345.45)); p += dot(p, p + 34.345); return fract(p.x * p.y); }
float vnoise(vec2 p) {
    vec2 i = floor(p), f = fract(p), u = f * f * (3.0 - 2.0 * f);
    float a = hash21(i), b = hash21(i + vec2(1,0)), c = hash21(i + vec2(0,1)), d = hash21(i + vec2(1,1));
    return mix(mix(a, b, u.x), mix(c, d, u.x), u.y);
}
float fbm(vec2 p) {
    float s = 0.0, a = 0.5;
    const mat2 m = mat2(1.6, 1.2, -1.2, 1.6);
    for (int i = 0; i < 5; ++i) { s += a * vnoise(p); p = m * p; a *= 0.5; }
    return s;
}
vec3 aces(vec3 x) { return clamp((x * (2.51 * x + 0.03)) / (x * (2.43 * x + 0.59) + 0.14), 0.0, 1.0); }

void main() {
    vec2 uv = (vUV * 2.0 - 1.0);
    uv.x *= uResolution.x / uResolution.y;

    vec3 RED    = themed(uPrimaryColour, BAD_RED);
    vec3 GOLD   = themed(uAccentColour,  BAD_GOLD);
    vec3 SHADOW = themed(uBackground,    BAD_SHADOW);

    float t = uTime * 0.08;               // slow drift
    float r = length(uv);

    // Beat pressure wave: a radial shock displaces the medium outward.
    float wave = sin(r * 6.0 - uTime * 3.0) * uBeat * 0.14 * exp(-r * 1.2);

    // Bass expands the plume (a lower sampling scale reads as a bigger volume).
    vec2 p = uv * (1.7 - uBass * 0.6) + vec2(0.0, -t * 2.0);
    p += wave;

    // Domain-warped fbm → churning smoke.
    vec2  q     = vec2(fbm(p + vec2(0.0, t)), fbm(p + vec2(5.2, 1.3) - t));
    float smoke = fbm(p + 2.5 * q + vec2(0.0, -t));
    smoke = pow(clamp(smoke, 0.0, 1.0), 1.8);

    float density = smoke * (0.45 + uEnergy * 0.9 + uBass * 0.5);

    vec3 col = mix(SHADOW * 0.4, RED, density);
    col += RED * pow(density, 3.0) * (1.0 + uBass * 2.0);   // hot glowing core

    // Treble embers: sparse gold flecks riding the smoke.
    vec2  ecell = floor(uv * 4.0 + vec2(0.0, t * 9.0));
    float e     = hash21(ecell);
    float ember = step(0.985 - uTreble * 0.03, e) * (0.5 + 0.5 * sin(uTime * 10.0 + e * 40.0));
    col += GOLD * ember * (0.6 + uTreble * 2.0) * smoothstep(0.05, 0.6, density);

    col += RED * uBeat * uBeat * 0.3 * exp(-r * r * 2.0);    // beat flash at core

    col += col * col * 0.6;                                  // heavy bloom
    col = aces(col * 1.2);
    col *= 1.0 - 0.42 * smoothstep(0.6, 1.6, r);             // vignette
    fragColor = vec4(col, 1.0);
}
