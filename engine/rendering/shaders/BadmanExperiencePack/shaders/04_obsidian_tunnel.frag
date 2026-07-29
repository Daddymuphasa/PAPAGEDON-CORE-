#version 460 core
// ─────────────────────────────────────────────────────────────────────────────
//  04 · OBSIDIAN TUNNEL  —  Badman Experience Pack Vol.1
//  An infinite black tunnel with red-illuminated ring edges. The camera travels
//  forward forever; beats expand the throat. Hypnotic, driving, tech-house.
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

    float expand = 1.0 + uBeat * 0.30 + uBass * 0.18;       // beat expands the throat
    float r = max(length(uv) / expand, 1e-3);
    float a = atan(uv.y, uv.x);

    float depth = 1.0 / r + uTime * (0.6 + uEnergy * 0.8);  // travel forward
    float rings = 0.5 + 0.5 * sin(depth * TAU);
    float edge  = pow(rings, 3.0);                          // bright ring edges
    float walls = fbm(vec2(a / TAU * 8.0, depth * 1.5));    // wall texture

    float lit = mix(walls * 0.25, edge, 0.72);
    lit *= smoothstep(0.0, 0.35, r);                        // dark far centre

    vec3 col = SHADOW * 0.3 * (1.0 - edge);
    col += RED  * lit * (0.6 + uEnergy);
    col += GOLD * pow(edge, 4.0) * (0.3 + uTreble);         // gold rim glints
    col += RED  * uBeat * uBeat * 0.4 * exp(-r * 3.0);      // throat flash

    col += col * col * 0.5;
    col = aces(col * 1.15);
    col *= 1.0 - 0.30 * smoothstep(0.8, 1.7, length(uv));
    fragColor = vec4(col, 1.0);
}
