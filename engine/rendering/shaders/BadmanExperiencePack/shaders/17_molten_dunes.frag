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
float terrain(vec2 p) { return sin(p.x*0.65+p.y*0.22)*0.6+sin(p.y*0.8)*0.28+noise(p*1.7)*0.15; }
void main() {
    vec2 uv=screen(); float t=uTime*0.75+uEvolutionPhase*0.06;
    vec3 ro=vec3(sin(t*0.11)*1.5,1.8,t*2.0);
    vec3 rd=normalize(vec3(uv.x,uv.y-0.45,1.6));
    float travel=0.0; vec3 p=ro;
    for(int i=0;i<42;++i) {
        p=ro+rd*travel; float d=p.y-terrain(p.xz);
        if(d<0.015 || travel>35.0) break;
        travel+=max(d*0.40,0.035);
    }
    vec3 col=uBackground+uPrimaryColour*exp(-abs(uv.y-0.3)*5.0)*0.20;
    if(travel<35.0) {
        float contour=pow(0.5+0.5*sin(terrain(p.xz)*42.0+uBass*2.0),14.0);
        float cracks=exp(-abs(noise(p.xz*2.0)-0.5)*95.0);
        col+=mix(uPrimaryColour,uAccentColour,contour)*(0.13+contour*0.65+cracks*uBeat)*exp(-travel*0.055);
    }
    fragColor=vec4(finish(col),1);
}
