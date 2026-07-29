#version 460 core
// ─────────────────────────────────────────────────────────────────────────────
//  08 · BLACK SUN  —  Badman Experience Pack Vol.1
//  A dark eclipse ringed by a burning corona. Energy waves ripple outward, bass
//  shakes the corona, treble throws sparks off the rim. Iconic, monolithic.
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
uniform vec3  uPrimaryColour;
uniform vec3  uSecondaryColour;
uniform vec3  uAccentColour;
uniform vec3  uBackground;

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

    float t = uTime;
    float r = length(uv);
    float a = atan(uv.y, uv.x);

    float radius = 0.42 + uBass * 0.05 * sin(t * 22.0);     // bass shakes the corona

    // Corona: noisy angular flares outside the disc.
    float flare  = pow(0.5 + 0.5 * sin(a * 40.0 + t * 0.5 + fbm(vec2(a * 3.0, t * 0.3)) * 5.0), 2.0);
    float corona = smoothstep(radius + 0.40, radius, r) * smoothstep(radius - 0.02, radius + 0.12, r);
    corona *= 0.4 + flare * 0.8;

    // Energy waves rippling out.
    float waves = 0.5 + 0.5 * sin(r * 20.0 - t * 4.0 - uBeat * 8.0);
    corona *= 0.6 + 0.4 * waves;

    float disc = smoothstep(radius, radius - 0.02, r);      // the black sun itself

    vec3 col = SHADOW * 0.30;
    col += RED  * corona * (0.8 + uEnergy);
    col += GOLD * pow(corona, 3.0) * (0.5 + uTreble);
    col = mix(col, BAD_BLACK * 0.15, disc);

    // Treble sparks flung off the rim.
    float rim   = smoothstep(0.03, 0.0, abs(r - radius));
    float spark = step(0.6, hash21(vec2(floor(a * 30.0), floor(t * 15.0))));
    col += BAD_CREAM * rim * spark * uTreble;

    col += col * col * 0.6;
    col = aces(col * 1.2);
    col *= 1.0 - 0.30 * smoothstep(0.8, 1.7, r);
    fragColor = vec4(col, 1.0);
}
