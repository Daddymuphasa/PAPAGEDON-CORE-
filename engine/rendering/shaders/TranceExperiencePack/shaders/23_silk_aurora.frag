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
    vec2 uv=screen(); float t=uTime*0.06+uEvolutionPhase*0.015;
    vec3 col=uBackground;
    for(int i=0;i<8;++i) {
        float f=float(i), x=uv.x+f*0.13;
        float y=0.45*sin(x*0.9+t*0.7+f*0.3)+0.16*sin(x*2.3-t*0.4)-0.45+f*0.12;
        float d=uv.y-y;
        float curtain=exp(-abs(d)*12.0)*exp(-max(d,0.0)*0.5);
        float pleats=pow(0.5+0.5*sin(x*55.0+fbm(vec2(x,t))*9.0+f),3.0);
        col+=mix(uSecondaryColour,uPrimaryColour,f/7.0)*curtain*(0.20+pleats*0.22);
        col+=uAccentColour*exp(-abs(d)*170.0)*(0.08+band(f/8.0)*0.24);
    }
    fragColor=vec4(finish(col),1);
}
