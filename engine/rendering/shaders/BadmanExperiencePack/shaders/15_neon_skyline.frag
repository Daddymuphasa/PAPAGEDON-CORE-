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
    vec2 uv=screen(); float t=uTime*0.35+uEvolutionPhase*0.04;
    vec3 col=uBackground+uPrimaryColour*exp(-abs(uv.y)*3.0)*0.08;
    for(int i=0;i<12;++i) {
        float f=float(i), z=1.0+f*0.45;
        vec2 q=vec2(uv.x*z+t*(1.0+f*0.08),uv.y*z+1.1);
        float cell=floor(q.x), h=hash(vec2(cell,f));
        float roof=0.35+h*1.8+band(fract(h))*0.55;
        float block=step(abs(fract(q.x)-0.5),0.40)*step(q.y,roof)*step(-0.5,q.y);
        float window=step(0.68,fract(q.x*6.0))*step(0.72,fract(q.y*7.0));
        vec3 building=palette(h)*((0.08+window*0.65)/(1.0+f*0.16));
        col=mix(col,building,block);
        col+=uAccentColour*exp(-abs(q.y-roof)*90.0)*block*0.35;
    }
    fragColor=vec4(finish(col),1);
}
