#version 330 core
in vec2 vUV;
out vec4 fragColor;
uniform float uTime, uEvolutionPhase, uBass, uMid, uTreble, uBeat, uEnergy;
uniform vec2 uResolution;
uniform vec3 uPrimaryColour, uSecondaryColour, uAccentColour, uBackground;
uniform sampler2D uSpectrum;
const float TAU = 6.28318530718;
float hash(vec2 p) { return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453); }
float noise(vec2 p) {
    vec2 i=floor(p), f=fract(p); f=f*f*(3.0-2.0*f);
    return mix(mix(hash(i),hash(i+vec2(1,0)),f.x),mix(hash(i+vec2(0,1)),hash(i+1.0),f.x),f.y);
}
float fbm(vec2 p) {
    float s=0.0, a=0.5;
    for(int i=0;i<4;++i) { s+=noise(p)*a; p=mat2(1.6,1.2,-1.2,1.6)*p+0.23; a*=0.5; }
    return s;
}
float band(float x) { return texture(uSpectrum,vec2(clamp(x,0.0,1.0),0.5)).r; }
vec2 screen() { return (vUV*2.0-1.0)*vec2(uResolution.x/max(uResolution.y,1.0),1.0); }
mat2 turn(float a) { return mat2(cos(a),-sin(a),sin(a),cos(a)); }
vec3 palette(float x) { return mix(uPrimaryColour,uAccentColour,clamp(x,0.0,1.0)); }
vec3 finish(vec3 c) { c=max(c,vec3(0)); return c/(0.7+c); }
void main() {
    vec2 uv=screen(); float t=uTime*0.045+uEvolutionPhase*0.03;
    vec3 col=uBackground+uSecondaryColour*fbm(uv*1.5+t*0.07)*0.2;
    for(int i=0;i<42;++i) {
        float f=float(i), seed=hash(vec2(f,4)); float depth=0.4+seed*2.5;
        vec2 pos=vec2((hash(vec2(f,7))*2.0-1.0)*2.7,
                      fract(hash(vec2(f,9))+t*(0.025+seed*0.03))*3.6-1.8)/depth;
        pos.x+=sin(t*0.5+f)*0.12;
        vec2 q=(uv-pos)*depth; float r=length(q);
        float core=exp(-r*r/(0.0004+0.0008*band(seed)));
        float halo=exp(-r*17.0)*0.12;
        col+=mix(uPrimaryColour,uAccentColour,seed)*(core+halo)*(0.5+uTreble*0.35);
    }
    fragColor=vec4(finish(col),1);
}
