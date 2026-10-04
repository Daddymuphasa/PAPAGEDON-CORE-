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
    vec2 uv=screen(); float t=uTime*0.055+uEvolutionPhase*0.025;
    float horizon=0.18+sin(t*0.13)*0.08;
    vec3 col=mix(uBackground,uPrimaryColour*0.3,exp(-abs(uv.y-horizon)*2.0));
    if(uv.y<horizon) {
        float depth=1.0/max(horizon-uv.y,0.08);
        vec2 water=vec2(uv.x*depth,depth+t*2.0);
        float a=fbm(water*0.8+vec2(0,t));
        float b=fbm(water*0.82-vec2(t*0.3,0));
        float caustic=pow(1.0-clamp(abs(a-b)*6.0,0.0,1.0),9.0);
        float reflect=exp(-uv.x*uv.x*(1.4+0.3*sin(t)));
        col+=uPrimaryColour*caustic*(0.1+0.35/(1.0+depth*0.03));
        col+=uAccentColour*reflect*caustic*(0.12+uBass*0.22);
    }
    vec2 moon=uv-vec2(0.65+sin(t*0.07)*0.2,0.62);
    col+=uAccentColour*exp(-dot(moon,moon)*65.0)*0.5;
    fragColor=vec4(finish(col),1);
}
