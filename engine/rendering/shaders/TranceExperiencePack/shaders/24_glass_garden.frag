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
    vec2 uv=screen(); float t=uTime*0.05+uEvolutionPhase*0.018;
    vec3 col=uBackground+uSecondaryColour*0.06;
    for(int i=0;i<11;++i) {
        float f=float(i), h=hash(vec2(f,3));
        vec2 pos=vec2((hash(vec2(f,2))*2.0-1.0)*1.8,(hash(vec2(f,5))*2.0-1.0)*0.8);
        pos+=vec2(sin(t+f),cos(t*0.71+f))*0.12;
        vec2 p=turn(t*0.14+f)*(uv-pos); float a=atan(p.y,p.x), r=length(p);
        float petal=0.11+0.09*cos(a*(4.0+floor(h*3.0)))+uBass*0.015;
        float glass=exp(-abs(r-petal)*95.0);
        float inner=exp(-abs(r-petal*0.55)*160.0);
        col+=palette(h)*(glass*0.22+inner*0.09)*(0.5+band(h));
    }
    fragColor=vec4(finish(col),1);
}
