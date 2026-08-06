#version 330 core
// ─────────────────────────────────────────────────────────────────────────────
//  12 · FINAL ASCENSION  —  Badman Experience Pack Vol.1
//  Pulse-tunnel hybrid: you are falling through a kaleidoscopic warp tunnel
//  while concentric shockwave rings explode outward on every kick. The walls
//  breathe with the bass, the whole frame lurches forward on the beat, and a
//  starburst core detonates at the vanishing point.
//  PAPAGEDON · dark-luxury rave visual system · designed for LED walls
// ─────────────────────────────────────────────────────────────────────────────
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

// ── Theme palette ───────────────────────────────────────────────────────────
vec3 palette(float t) {
    t = clamp(t, 0.0, 1.0);
    return t < 0.5 ? mix(uSecondaryColour, uPrimaryColour, t * 2.0)
                   : mix(uPrimaryColour,   uAccentColour,  (t - 0.5) * 2.0);
}

// ── Noise toolkit ───────────────────────────────────────────────────────────
float hash21(vec2 p) {
    p = fract(p * vec2(123.34, 345.45));
    p += dot(p, p + 34.345);
    return fract(p.x * p.y);
}

float vnoise(vec2 p) {
    vec2 i = floor(p), f = fract(p), u = f * f * (3.0 - 2.0 * f);
    float a = hash21(i), b = hash21(i + vec2(1,0));
    float c = hash21(i + vec2(0,1)), d = hash21(i + vec2(1,1));
    return mix(mix(a, b, u.x), mix(c, d, u.x), u.y);
}

float fbm(vec2 p) {
    float s = 0.0, a = 0.5, f = 1.0;
    float g = clamp(0.5 + 0.12 * (uDetail - 1.0), 0.38, 0.66);
    for (int i = 0; i < 4; ++i) { s += a * vnoise(p * f); f *= 2.0; a *= g; }
    return s;
}

float warpedFbm(vec2 p, float t) {
    vec2 q = vec2(fbm(p + vec2(0.0, t * 0.12)),
                  fbm(p + vec2(5.2, 1.3) - vec2(t * 0.09, 0.0)));
    return fbm(p + uDistortion * q);
}

vec3 aces(vec3 x) {
    return clamp((x * (2.51 * x + 0.03)) / (x * (2.43 * x + 0.59) + 0.14), 0.0, 1.0);
}

void main() {
    vec2 uv = (vUV * 2.0 - 1.0);
    uv.x *= uResolution.x / uResolution.y;

    // ── Physical beat pump ──────────────────────────────────────────────────
    // Hard kick punches the frame inward; bass breathes; energy pushes the
    // whole scene toward the camera so drops rush in with depth.
    float kick = uBeat * uBeat;
    float pump = 1.0 - kick * 0.16 - uBass * 0.09 - uEnergy * 0.06;
    uv *= pump;

    float t = uTime * (0.3 + uEnergy * 0.2) * uMotion;
    float r = max(length(uv), 1e-3);
    float ang = atan(uv.y, uv.x);

    // ── 1. Tunnel depth rush ────────────────────────────────────────────────
    // 1/r perspective: you are accelerating into infinity.  The beat lurches
    // forward, the energy controls cruise speed, the bass warps the walls.
    float depth = 1.0 / r + t * (1.4 + uEnergy * 2.0) + uBeat * 1.2;
    float tunnelWalls = warpedFbm(
        vec2(ang / kPi * 4.0 + uBass * 0.3, depth * 2.5), t);
    float tunnelRings = 0.5 + 0.5 * sin(depth * kTau * (1.2 + uMid * 0.8));
    float tunnel = mix(tunnelWalls, tunnelRings, 0.45 + uEnergy * 0.15);
    tunnel *= smoothstep(0.0, 0.30, r);

    // ── 2. Kaleidoscope pulse shockwaves ────────────────────────────────────
    // Mirror the tunnel into wedges so the walls kaleidoscope.  Layer
    // concentric shockwaves that expand on every kick.
    float wedges = 8.0 + floor(uMid * 6.0);
    float wedge = abs(fract(ang / kTau * wedges) - 0.5);

    float wave1 = 0.5 + 0.5 * sin(r * (16.0 + uBass * 22.0) - t * 7.0 - uBeat * 10.0);
    float wave2 = 0.5 + 0.5 * sin(r * (9.0 + uBass * 12.0) - t * 4.5 - uBeat * 6.0);
    float waves = max(wave1, wave2 * 0.7);

    float kTex = warpedFbm(vec2(wedge * 7.0, r * 5.0 - t * 1.2), t * 1.1);
    float pulse = waves * (0.35 + 0.65 * kTex);
    pulse *= smoothstep(1.5, 0.08, r);

    // ── 3. Beat-triggered burst ─────────────────────────────────────────────
    // A supernova detonation at the vanishing point: an explosive radial
    // burst that fires on every kick and decays fast.
    float burst = uBeat * exp(-r * 2.5) * 2.0;

    // Starburst rays fanning out from centre on the beat.
    float rays = pow(0.5 + 0.5 * sin(ang * 12.0 + t * 0.4), 4.0);
    burst += uBeat * rays * exp(-r * 1.8) * 0.8;

    // ── 4. Spiral energy flow ───────────────────────────────────────────────
    // A logarithmic spiral underlay that responds to mid/treble, adding
    // rotational motion so the tunnel feels alive and spinning.
    float spiral = ang * 3.0 + log(r + 0.05) * 5.0
                 - t * (1.5 + uEnergy * 2.5) - uBeat * 2.0;
    float arms = pow(0.5 + 0.5 * sin(spiral), 2.0) * 0.35;
    arms *= smoothstep(1.3, 0.15, r);

    // ── Combine layers ──────────────────────────────────────────────────────
    float pattern = tunnel * 0.45 + pulse * 0.40 + arms * 0.15 + burst;
    pattern = clamp(pattern, 0.0, 1.0);
    pattern = smoothstep(0.02, 0.88, pattern);

    // ── Colour ──────────────────────────────────────────────────────────────
    // Bass pushes the palette lookup hotter; the burst pushes toward accent.
    vec3 color = palette(pattern + uBass * 0.20 + burst * 0.3);

    float luma = dot(color, vec3(0.299, 0.587, 0.114));
    float sat  = clamp(uSaturationBase + 0.3 + uIntensity * uSaturationScale, 0.0, 1.35);
    color = mix(vec3(luma), color, sat);

    float brightness = (0.14 + uEnergy * 1.2) * uMasterExposure;
    color *= brightness * (0.45 + 0.8 * pattern);

    // Ambient background fills the darkest regions.
    color += uBackground * (1.0 - pattern) * 1.3;

    // ── Beat swell ──────────────────────────────────────────────────────────
    // The kick makes existing colours bloom and the bass adds a warm lift.
    color += color * kick * 1.0;
    color += color * uBass * 0.35;

    // Theme glow + bloom.
    color += color * uGlow * uMasterGlow;
    color += color * color * uBloom * 1.2;

    // Mood tint.
    color *= mix(vec3(0.9, 1.0, 1.1), vec3(1.1, 1.0, 0.9), clamp(uMood, 0.0, 1.0));

    // Film grain.
    if (uNoise > 0.001) {
        float gr = hash21(gl_FragCoord.xy + fract(uTime) * 137.0) - 0.5;
        color += gr * uNoise * 0.22;
    }

    // Vignette — light, keeps the visual filling the wall.
    float d = length(uv);
    color *= 1.0 - 0.28 * smoothstep(0.8, 1.7, d);

    color *= uMasterBrightness;
    color = aces(color * 1.2);
    fragColor = vec4(clamp(color, 0.0, 1.0), 1.0);
}
