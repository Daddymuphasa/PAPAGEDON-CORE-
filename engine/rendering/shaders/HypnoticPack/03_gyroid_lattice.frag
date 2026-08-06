#version 330 core
// Gyroid Lattice - PAPAGEDON Hypnotic Pack
// Deep, infinite-detail visuals for the Badman Experience: fractals, kaleidoscope
// tunnels and liquid metal in red/gold. Physical beat, adaptive-audio reactive.
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
uniform vec3  uPrimaryColour;
uniform vec3  uSecondaryColour;
uniform vec3  uAccentColour;
uniform vec3  uBackground;
uniform float uGlow;
uniform float uBloom;
uniform float uMotion;
uniform float uNoise;
uniform float uDistortion;
uniform float uSaturationBase;
uniform float uSaturationScale;
uniform float uDetail;
uniform float uMasterBrightness;
uniform float uMasterGlow;
uniform float uMasterExposure;

const float kPi  = 3.14159265358979;
const float kTau = 6.28318530717959;

vec3 palette(float t) {
    t = clamp(t, 0.0, 1.0);
    return t < 0.5 ? mix(uSecondaryColour, uPrimaryColour, t * 2.0)
                   : mix(uPrimaryColour,   uAccentColour,  (t - 0.5) * 2.0);
}
float hash21(vec2 p) { p = fract(p * vec2(123.34, 345.45)); p += dot(p, p + 34.345); return fract(p.x * p.y); }
float vnoise(vec2 p) {
    vec2 i = floor(p), f = fract(p), u = f * f * (3.0 - 2.0 * f);
    float a = hash21(i), b = hash21(i + vec2(1,0)), c = hash21(i + vec2(0,1)), d = hash21(i + vec2(1,1));
    return mix(mix(a, b, u.x), mix(c, d, u.x), u.y);
}
float fbm(vec2 p) {
    float s = 0.0, a = 0.5, f = 1.0;
    float g = clamp(0.5 + 0.12 * (uDetail - 1.0), 0.38, 0.66);
    for (int i = 0; i < 5; ++i) { s += a * vnoise(p * f); f *= 2.0; a *= g; }
    return s;
}
float warpedFbm(vec2 p, float t) {
    vec2 q = vec2(fbm(p + vec2(0.0, t * 0.10)), fbm(p + vec2(5.2, 1.3) - vec2(t * 0.08, 0.0)));
    return fbm(p + uDistortion * q);
}
mat2 rot(float a) { float c = cos(a), s = sin(a); return mat2(c, -s, s, c); }
vec3 aces(vec3 x) { return clamp((x*(2.51*x+0.03))/(x*(2.43*x+0.59)+0.14), 0.0, 1.0); }
vec3 finish(vec3 col, vec2 uv, float kick) {
    col += col * kick * 0.7;
    col += col * uGlow * uMasterGlow;
    col += col * col * uBloom;
    col *= mix(vec3(0.9,1.0,1.1), vec3(1.1,1.0,0.9), clamp(uMood, 0.0, 1.0));
    col *= uMasterExposure;
    col *= 1.0 - 0.3 * smoothstep(0.95, 1.75, length(uv));
    col *= uMasterBrightness;
    return aces(col * 1.2);
}


// Gyroid: an infinite organic 3D lattice, surface-rendered with normal shading and
// gold fresnel edges — flying through it reads like a red metallic honeycomb.
float gmap(vec3 p) { return abs(dot(sin(p), cos(p.zxy))) * 0.5 - 0.08; }
vec3 gnorm(vec3 p) {
    vec2 e = vec2(0.012, 0.0);
    return normalize(vec3(gmap(p + e.xyy) - gmap(p - e.xyy),
                          gmap(p + e.yxy) - gmap(p - e.yxy),
                          gmap(p + e.yyx) - gmap(p - e.yyx)));
}
void main() {
    vec2 uv = (vUV * 2.0 - 1.0); uv.x *= uResolution.x / uResolution.y;
    float kick = uBeat * uBeat;
    float t = uTime * (0.12 + uEnergy * 0.14) * uMotion;
    float scale = 1.4 + uMid * 0.4;
    vec3 ro = vec3(0.0, 0.0, t * 1.5);
    vec3 rd = normalize(vec3(uv, 1.5 - kick * 0.3));
    float tt = 0.1;
    bool hit = false;
    vec3 p = ro;
    for (int i = 0; i < 80; ++i) {
        p = ro + rd * tt;
        p.xy = rot(t * 0.15) * p.xy;
        float d = gmap(p * scale) / scale;
        if (d < 0.002) { hit = true; break; }
        tt += d * 0.6;
        if (tt > 20.0) break;
    }
    vec3 col = uBackground * 0.35;
    if (hit) {
        vec3 n = gnorm(p * scale);
        float fres = pow(1.0 - abs(dot(n, rd)), 3.0);
        float diff = 0.4 + 0.6 * (0.5 + 0.5 * n.y);
        float fog  = exp(-tt * 0.11);
        col = palette(diff * 0.7 + uBass * 0.12) * (0.3 + uEnergy * 0.9);
        col += uAccentColour * fres * (0.5 + uTreble);     // gold rim highlights
        col += vec3(1.0, 0.9, 0.7) * pow(fres, 3.0) * 0.3; // hot metal glints
        col *= fog;
    }
    fragColor = vec4(finish(col, uv, kick), 1.0);
}
