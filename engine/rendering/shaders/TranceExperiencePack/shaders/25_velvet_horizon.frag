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
    vec2 uv=screen(); float t=uTime*0.04+uEvolutionPhase*0.02;
    vec3 col=mix(uBackground,uPrimaryColour*0.22,exp(-abs(uv.y-0.05)*2.0));
    for(int i=0;i<9;++i) {
        float f=float(i), depth=1.0+f*0.27;
        float y=-0.8+f*0.15+sin(uv.x/depth*2.0+t/depth+f)*0.13;
        y+=sin(uv.x*3.2/depth-t*0.7)*0.035;
        float body=1.0-smoothstep(y-0.015,y+0.015,uv.y);
        col=mix(col,mix(uSecondaryColour,uPrimaryColour,f/12.0)*(0.15+f*0.015),body*0.8);
        col+=uAccentColour*exp(-abs(uv.y-y)*150.0)*(0.08+uTreble*0.13);
    }
    float mist=fbm(vec2(uv.x*1.5+t,uv.y*3.0))*exp(-abs(uv.y)*3.0);
    col+=uPrimaryColour*mist*0.13;
    fragColor=vec4(finish(col),1);
}
