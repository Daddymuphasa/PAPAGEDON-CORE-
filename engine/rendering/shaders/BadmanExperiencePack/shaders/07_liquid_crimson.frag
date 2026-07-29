#version 460 core
// ─────────────────────────────────────────────────────────────────────────────
//  07 · LIQUID CRIMSON  —  Badman Experience Pack Vol.1
//  Molten plasma and smoke folded together — organic, slow, hypnotic. Mid
//  frequencies distort the flow, gold veins glint through the crimson.
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

    float t    = uTime * 0.15;
    float warp = 0.6 + uMid * 1.3;                          // mid distorts the flow
    vec2  p    = uv * 1.5;

    // Iterative domain warp → folding fluid.
    vec2  q = vec2(fbm(p + t), fbm(p + vec2(3.1, 1.7) - t));
    vec2  r = vec2(fbm(p + warp * q + vec2(1.7, 9.2) + 0.15 * t),
                   fbm(p + warp * q + vec2(8.3, 2.8) - 0.12 * t));
    float f     = fbm(p + warp * r);
    float veins = abs(fbm(p * 1.3 + r) - f);                // ridged caustic veins

    float liquid = pow(clamp(f, 0.0, 1.0), 1.5);

    vec3 col = mix(SHADOW * 0.4, RED, liquid);
    col += RED  * smoothstep(0.0, 0.22, veins) * (0.6 + uEnergy);
    col += GOLD * pow(1.0 - veins, 8.0) * 0.35 * (0.4 + uMid);   // gold glints
    col += RED  * uBeat * 0.2;

    col += col * col * 0.55;
    col = aces(col * 1.15);
    col *= 1.0 - 0.35 * smoothstep(0.7, 1.6, length(uv));
    fragColor = vec4(col, 1.0);
}
