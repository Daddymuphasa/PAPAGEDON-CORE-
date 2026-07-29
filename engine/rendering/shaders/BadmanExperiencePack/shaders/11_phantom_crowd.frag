#version 460 core
// ─────────────────────────────────────────────────────────────────────────────
//  11 · PHANTOM CROWD  —  Badman Experience Pack Vol.1
//  Abstract crowd silhouettes against a red backlit stage, bobbing to the beat
//  through drifting smoke. Pure shadow shapes — never actual people.
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

// One abstract silhouette row: bobbing rounded "shoulders + head" blobs, no detail.
float crowdRow(vec2 uv, float baseY, float scale, float speed, float beat) {
    float x    = uv.x * scale;
    float cell = floor(x);
    float fx   = fract(x) - 0.5;
    float h    = hash21(vec2(cell, baseY * 7.0));
    float bob  = 0.03 * sin(uTime * (2.5 + h * 4.0) + h * TAU) * (0.5 + beat * 1.5);
    float y    = uv.y - (baseY + bob);
    float head = smoothstep(0.30, 0.24, length(vec2(fx, (y - 0.14) * 2.1)));
    float body = smoothstep(0.48, 0.40, abs(fx)) * step(y, 0.06);
    return clamp(max(head, body), 0.0, 1.0);
}

void main() {
    vec2 uv = (vUV * 2.0 - 1.0);
    uv.x *= uResolution.x / uResolution.y;

    vec3 RED    = themed(uPrimaryColour, BAD_RED);
    vec3 GOLD   = themed(uAccentColour,  BAD_GOLD);
    vec3 SHADOW = themed(uBackground,    BAD_SHADOW);

    float t = uTime;

    // Backlit stage glow + smoke behind the crowd.
    vec3 col = SHADOW * 0.25;
    col += RED  * exp(-abs(uv.y - 0.15) * 2.4) * (0.5 + uBass * 0.9);
    col += GOLD * exp(-length(uv - vec2(0.0, 0.35)) * 3.0) * (0.2 + uBeat * 0.6);
    col += RED  * fbm(uv * 2.0 - t * 0.4) * 0.10;

    // Three receding rows of silhouettes along the lower frame.
    float crowd = 0.0;
    crowd = max(crowd, crowdRow(uv, -0.35,  8.0, 0.0, uBeat) * 1.0);
    crowd = max(crowd, crowdRow(uv, -0.55, 12.0, 0.0, uBeat) * 0.85);
    crowd = max(crowd, crowdRow(uv, -0.78, 16.0, 0.0, uBeat) * 0.7);
    crowd *= step(uv.y, 0.05);

    // Silhouettes read as deep shadow; a faint red rim from the backlight.
    col = mix(col, BAD_BLACK * 0.12, crowd);
    col += RED * crowd * smoothstep(0.0, 0.4, uEnergy) * 0.10;

    col += col * col * 0.5;
    col = aces(col * 1.15);
    col *= 1.0 - 0.30 * smoothstep(0.8, 1.7, length(uv));
    fragColor = vec4(col, 1.0);
}
