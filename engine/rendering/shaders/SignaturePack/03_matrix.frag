#version 460 core
// Matrix - PAPAGEDON Signature Pack
// The original signature form, as a standalone shader. Physical beat response:
// the frame pumps on the kick and the colours swell - no white strobe.
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
    for (int i = 0; i < 3; ++i) { s += a * vnoise(p * f); f *= 2.0; a *= g; }
    return s;
}
float warpedFbm(vec2 p, float t) {
    vec2 q = vec2(fbm(p + vec2(0.0, t * 0.10)), fbm(p + vec2(5.2, 1.3) - vec2(t * 0.08, 0.0)));
    return fbm(p + uDistortion * q);
}
vec3 aces(vec3 x) { return clamp((x*(2.51*x+0.03))/(x*(2.43*x+0.59)+0.14), 0.0, 1.0); }


float patMatrix(vec2 uv, float t) {
    float cols = 40.0;
    float col = floor((uv.x * 0.5 + 0.5) * cols);
    float y = uv.y * 0.5 + 0.5;
    float speed = 0.25 + hash21(vec2(col, 3.0)) * (0.5 + uMid * 1.5);
    float head = fract(t * speed + hash21(vec2(col, 7.0)));
    float d = fract(head - y);
    float stream = pow(1.0 - d, 3.0);
    float cell = hash21(vec2(col, floor(y * cols)));
    float flick = 0.55 + 0.45 * sin(t * 9.0 + cell * 33.0);
    return clamp(stream * flick * (0.7 + uEnergy * 0.6), 0.0, 1.0);
}

void main() {
    vec2 uv = (vUV * 2.0 - 1.0);
    uv.x *= uResolution.x / uResolution.y;

    // Physical beat: a sharp kick punches the frame inward, the low end breathes.
    float kick = uBeat * uBeat;
    float pump = 1.0 - kick * 0.09 - uBass * 0.05;
    uv *= pump;

    float t = uTime * 0.25 * uMotion;

    float pattern = clamp(patMatrix(uv, t), 0.0, 1.0);
    pattern = smoothstep(0.03, 0.9, pattern);

    vec3 color = palette(pattern + uBass * 0.15);

    float luma = dot(color, vec3(0.299, 0.587, 0.114));
    float sat  = clamp(uSaturationBase + 0.2 + uIntensity * uSaturationScale, 0.0, 1.25);
    color = mix(vec3(luma), color, sat);

    float brightness = (0.2 + uEnergy * 0.9) * uMasterExposure;
    color *= brightness * (0.55 + 0.7 * pattern);

    color += uBackground * (1.0 - pattern) * 1.4;

    // Beat SWELL, not a flash: the existing colours bloom on the kick and decay
    // smoothly; the bass adds a low, warm lift you feel more than see.
    color += color * kick * 0.7;
    color += color * uBass * 0.25;

    color += color * uGlow * uMasterGlow;
    color += color * color * uBloom;
    color *= mix(vec3(0.9, 1.0, 1.1), vec3(1.1, 1.0, 0.9), clamp(uMood, 0.0, 1.0));
    if (uNoise > 0.001) { float gr = hash21(gl_FragCoord.xy + fract(uTime) * 137.0) - 0.5; color += gr * uNoise * 0.2; }
    float d = length(uv);
    color *= 1.0 - 0.3 * smoothstep(0.85, 1.7, d);
    color *= uMasterBrightness;
    color = aces(color * 1.15);
    fragColor = vec4(clamp(color, 0.0, 1.0), 1.0);
}
