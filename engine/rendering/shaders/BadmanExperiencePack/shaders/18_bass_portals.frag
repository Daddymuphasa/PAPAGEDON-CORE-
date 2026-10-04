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
    vec2 uv=screen(); float t=uTime*0.7+uEvolutionPhase*0.08;
    uv+=vec2(sin(t*0.23)*0.28,cos(t*0.19)*0.16);
    vec3 col=uBackground;
    for(int i=0;i<18;++i) {
        float f=float(i), depth=mod(f-t*1.3,18.0)+0.8;
        vec2 q=turn(depth*0.13+t*0.1)*uv*depth*0.32;
        float hex=max(abs(q.x)*0.866+abs(q.y)*0.5,abs(q.y));
        float distance=abs(hex-(0.85+uBass*0.07));
        float edge=exp(-distance*150.0/depth);
        float rays=0.4+0.6*band(fract(atan(q.y,q.x)/TAU+0.5));
        col+=palette(fract(f*0.16))*edge*rays/(1.0+depth*0.15)*(0.5+uBeat*0.3);
    }
    fragColor=vec4(finish(col),1);
}
