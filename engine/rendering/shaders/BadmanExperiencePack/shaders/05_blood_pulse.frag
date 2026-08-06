#version 330 core
// ─────────────────────────────────────────────────────────────────────────────
//  05 · BLOOD PULSE  —  Badman Experience Pack Vol.1
//  The whole world breathes. A heartbeat compresses the environment, bloom
//  spikes on every thump. No particles — pure atmospheric pressure.
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

    // Heartbeat: a slow breath under a sharp on-beat thump and bass swell.
    float breathe = 0.5 + 0.5 * sin(uTime * 1.6);
    float pulse   = uBeat + breathe * 0.30 + uBass * 0.5;

    vec2  p = uv * (1.0 - pulse * 0.15);                    // world compresses
    float r = length(p);

    float field = fbm(p * 2.0 + uTime * 0.10);
    float glow  = exp(-r * r * (1.6 - pulse * 0.6));

    vec3 col = SHADOW * 0.35;
    col += RED  * glow * (0.7 + pulse * 1.2);
    col += RED  * field * 0.25 * (0.5 + uEnergy);
    col += GOLD * pow(glow, 5.0) * (0.3 + uBeat);           // molten centre

    col += col * col * (0.4 + pulse * 0.6);                 // bloom spikes with the beat
    col = aces(col * 1.2);
    col *= 1.0 - 0.42 * smoothstep(0.5, 1.5, length(uv));
    fragColor = vec4(col, 1.0);
}
