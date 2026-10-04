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
float vault(vec3 p) {
    p.xy=turn(p.z*0.08+sin(uTime*0.09)*0.2)*p.xy;
    vec3 cell=mod(p+2.0,4.0)-2.0;
    vec3 d=abs(cell)-vec3(0.12+uBass*0.05,1.45,0.12);
    return length(max(d,0.0))+min(max(d.x,max(d.y,d.z)),0.0);
}
void main() {
    vec2 uv=screen(); vec3 ro=vec3(sin(uTime*0.13)*0.7,cos(uTime*0.09)*0.3,uEvolutionPhase*0.6+uTime);
    vec3 rd=normalize(vec3(uv,1.8)); float travel=0.0, glow=0.0;
    for(int i=0;i<40;++i) {
        vec3 p=ro+rd*travel; float d=vault(p);
        glow+=exp(-abs(d)*25.0)*0.045;
        if(d<0.006 || travel>28.0) break;
        travel+=max(d*0.65,0.035);
    }
    vec3 hit=ro+rd*travel;
    vec3 col=uBackground+palette(fract(hit.z*0.11))*glow*(0.45+uBass);
    col+=uSecondaryColour*exp(-travel*0.055)*0.35;
    fragColor=vec4(finish(col),1);
}
