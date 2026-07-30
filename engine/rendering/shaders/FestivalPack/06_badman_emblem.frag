#version 460 core
// Badman Emblem - PAPAGEDON Festival Pack
// Inspired by the Badman Experience 4.0 "Festival of Sounds" flyer: red stage
// lights, crimson fog, gold accents. Physical beat, adaptive-audio reactive.
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
    for (int i = 0; i < 4; ++i) { s += a * vnoise(p * f); f *= 2.0; a *= g; }
    return s;
}
float warpedFbm(vec2 p, float t) {
    vec2 q = vec2(fbm(p + vec2(0.0, t * 0.10)), fbm(p + vec2(5.2, 1.3) - vec2(t * 0.08, 0.0)));
    return fbm(p + uDistortion * q);
}
vec3 aces(vec3 x) { return clamp((x*(2.51*x+0.03))/(x*(2.43*x+0.59)+0.14), 0.0, 1.0); }
vec3 finish(vec3 col, vec2 uv, float kick) {
    col += col * kick * 0.7;
    col += col * uGlow * uMasterGlow;
    col += col * col * uBloom;
    col *= mix(vec3(0.9,1.0,1.1), vec3(1.1,1.0,0.9), clamp(uMood, 0.0, 1.0));
    col *= uMasterExposure;
    col *= 1.0 - 0.3 * smoothstep(0.9, 1.7, length(uv));
    col *= uMasterBrightness;
    return aces(col * 1.2);
}


void main() {
    vec2 uv = (vUV * 2.0 - 1.0); uv.x *= uResolution.x / uResolution.y;
    float kick = uBeat * uBeat;
    uv *= 1.0 - kick * 0.12 - uEnergy * 0.05;
    float r = length(uv); float a = atan(uv.y, uv.x);
    float t = uTime * 0.4 * uMotion;
    float rays = pow(0.5 + 0.5 * sin(a * 24.0 + t + sin(a * 6.0 + t) * 0.5), 2.0);
    float radius = 0.28 + uBass * 0.04 * sin(t * 10.0);
    float disc = smoothstep(radius, radius - 0.02, r);
    float corona = smoothstep(radius + 0.5, radius, r) * smoothstep(radius - 0.02, radius + 0.1, r);
    corona *= 0.4 + rays * 0.8;
    vec3 col = uBackground * 0.3;
    col += uPrimaryColour * corona * (0.7 + uEnergy);
    col += uAccentColour  * pow(corona, 2.5) * (0.5 + uTreble);
    col += mix(uAccentColour, vec3(1.0), 0.4) * disc * (0.4 + kick);
    col += uPrimaryColour * exp(-r * r * 4.0) * (0.3 + kick * 0.6);
    fragColor = vec4(finish(col, uv, kick), 1.0);
}
