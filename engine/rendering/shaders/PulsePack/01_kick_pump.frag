#version 330 core
// Kick Pump - PAPAGEDON Pulse Pack
// Reacts to the kick physically - the scene pumps, heaves and swells with the
// beat and bass. No white strobe flashes.
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


void main() {
    vec2 uv = (vUV * 2.0 - 1.0); uv.x *= uResolution.x / uResolution.y;
    float kick = uBeat * uBeat;
    uv *= 1.0 - kick * 0.18 - uBass * 0.10;          // whole field breathes with the kick
    float r = length(uv);
    float t = uTime * 0.3 * uMotion;
    float rings = 0.5 + 0.5 * sin(r * (8.0 + uBass * 6.0) - t * 3.0);
    float core  = exp(-r * r * (2.5 - kick * 1.2));  // core swells on the kick
    float body  = warpedFbm(uv * 2.0 + t * 0.2, t);
    float g = clamp(core * 1.2 + rings * 0.4 * smoothstep(1.2, 0.1, r) + body * 0.25, 0.0, 1.0);
    g = smoothstep(0.05, 0.95, g);
    vec3 color = palette(g + uBass * 0.15);
    float luma = dot(color, vec3(0.299, 0.587, 0.114));
    color = mix(vec3(luma), color, clamp(uSaturationBase + 0.25 + uIntensity * uSaturationScale, 0.0, 1.25));
    color *= (0.2 + uEnergy * 0.9) * uMasterExposure * (0.5 + 0.7 * g);
    color += uBackground * (1.0 - g) * 1.4;
    color += color * kick * 0.9; color += color * uBass * 0.3;
    color += color * uGlow * uMasterGlow; color += color * color * uBloom;
    color *= mix(vec3(0.9,1.0,1.1), vec3(1.1,1.0,0.9), clamp(uMood, 0.0, 1.0));
    color *= 1.0 - 0.3 * smoothstep(0.85, 1.7, r);
    color *= uMasterBrightness; color = aces(color * 1.2);
    fragColor = vec4(clamp(color, 0.0, 1.0), 1.0);
}
