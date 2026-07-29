#version 460 core
// ─────────────────────────────────────────────────────────────────────────────
//  12 · FINAL ASCENSION  —  Badman Experience Pack Vol.1
//  The climax combination: tunnel + smoke + laser sheets + expanding energy
//  rings + rising particles + a cream ascension core, all blended and bloomed.
//  Built for the drop at the peak of the set.
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
    float r = max(length(uv), 1e-3);
    float a = atan(uv.y, uv.x);

    vec3 col = SHADOW * 0.30;

    // 1 · Tunnel depth rings.
    float depth = 1.0 / r + t * (0.8 + uEnergy);
    float rings = pow(0.5 + 0.5 * sin(depth * TAU), 2.0);
    col += RED * rings * smoothstep(0.0, 0.3, r) * 0.5;

    // 2 · Energy rings expanding on the beat.
    float ering = sin(r * 14.0 - t * 4.0 - uBeat * 10.0);
    col += GOLD * smoothstep(0.7, 1.0, ering) * exp(-r) * (0.4 + uBeat);

    // 3 · Rotating laser sheets (treble spins, bass thickens).
    float rot = t * 0.5 + uTreble * 2.0;
    mat2  rm  = mat2(cos(rot), -sin(rot), sin(rot), cos(rot));
    vec2  lp  = rm * uv;
    float laser = 0.0;
    for (int i = 0; i < 4; ++i) {
        float off = sin(t * 1.2 + float(i) * 1.9) * 0.7;
        laser += smoothstep(0.04 + uBass * 0.10, 0.0, abs(lp.y - off));
    }
    col += RED * laser * (0.5 + uEnergy);

    // 4 · Smoke wash.
    vec2  q     = vec2(fbm(uv * 1.5 + t * 0.1), fbm(uv * 1.5 + vec2(5.0) - t * 0.1));
    float smoke = fbm(uv * 1.5 + 2.0 * q);
    col += RED * smoke * 0.20 * (0.5 + uBass);

    // 5 · Rising gold particles.
    vec2  pp = uv * 10.0 + vec2(0.0, t * 3.0);
    float pc = hash21(floor(pp));
    col += GOLD * step(0.97, pc) * smoothstep(0.2, 0.0, length(fract(pp) - 0.5)) * (0.4 + uTreble);

    // 6 · Ascension core.
    col += BAD_CREAM * exp(-r * r * 3.0) * (0.4 + uBeat * 0.8 + uEnergy * 0.5);

    col += col * col * 0.7;                                 // heavy bloom
    col = aces(col * 1.3);
    col *= 1.0 - 0.30 * smoothstep(0.8, 1.7, r);
    fragColor = vec4(col, 1.0);
}
