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
    vec2 uv=screen(); float t=uTime*0.42+uEvolutionPhase*0.06;
    vec3 col=uBackground;
    for(int i=0;i<24;++i) {
        float f=float(i), h=hash(vec2(f,9));
        vec2 origin=vec2(mix(-2.2,2.2,step(0.5,h)),-0.9+hash(vec2(f,2))*0.35);
        float angle=0.3+sin(t*0.4+f)*0.55+f*0.06;
        vec2 dir=normalize(vec2(-sign(origin.x),1.2+sin(angle)*0.9));
        vec2 p=uv-origin; float along=dot(p,dir);
        float distance=abs(p.x*dir.y-p.y*dir.x);
        float beam=exp(-distance*180.0)+exp(-distance*20.0)*0.06;
        float pulse=0.3+0.7*pow(0.5+0.5*sin(along*2.0-t*6.0+f),5.0);
        col+=palette(h)*beam*step(0.0,along)*pulse*(0.14+band(h)*0.8+uBeat*0.18);
    }
    fragColor=vec4(finish(col),1);
}
