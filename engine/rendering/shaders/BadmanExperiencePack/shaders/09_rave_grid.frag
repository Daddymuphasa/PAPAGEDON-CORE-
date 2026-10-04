#version 330 core
// ─────────────────────────────────────────────────────────────────────────────
//  09 · RAVE GRID  —  Badman Experience Pack Vol.1
//  Minimal industrial geometry: a neon-red wireframe floor racing to a hazy
//  horizon. Perspective distortion, light haze, beats bend the grid. Tech house.
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
uniform float uEvolutionPhase;
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

    float t = uTime * 0.55 + uEvolutionPhase * 0.12;
    vec3 teal = themed(uSecondaryColour, vec3(0.01, 0.48, 0.55));
    vec2 grid = uv * 4.5;
    grid.x += sin(grid.y * 0.34 + t * 0.27) * 0.55;
    grid.y += t * 0.18;
    float row = floor(grid.y);
    grid.x += mod(row, 2.0) * 0.5;
    vec2 cell = floor(grid);
    vec2 p = fract(grid) - 0.5;
    float seed = hash21(cell);
    float groove = 0.5 + 0.5 * sin(cell.x * 0.85 + cell.y * 1.2 - t * 2.5);
    float rhythm = mix(uBass, uTreble, step(0.5, seed));
    float size = 0.13 + groove * 0.16 + rhythm * 0.11;
    float box = max(abs(p.x), abs(p.y));
    float edge = exp(-abs(box - size) * 62.0);
    float fill = (1.0 - smoothstep(size - 0.025, size, box)) * groove * 0.20;
    vec3 tile = mix(teal * 2.0, RED, step(0.42, seed));
    tile = mix(tile, GOLD, step(0.85, seed));
    vec3 col = SHADOW * 0.3 + tile * (edge + fill) * (0.55 + rhythm + uBeat * 0.65);
    float sweep = exp(-pow(sin(uv.x * 0.7 - uv.y * 0.4 - t * 0.25) * 7.0, 2.0));
    col += tile * sweep * edge * 0.5;
    col = aces(col * 1.2);
    fragColor = vec4(col, 1.0);
}
