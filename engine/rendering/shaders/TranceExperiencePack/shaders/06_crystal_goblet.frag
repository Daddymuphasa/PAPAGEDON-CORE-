#version 330 core
in vec2 vUV;
out vec4 fragColor;
uniform float uTime;
uniform vec2  uResolution;
uniform float uBass;
uniform float uMid;
uniform float uTreble;
uniform float uBeat;
uniform float uEnergy;
uniform float uIntensity;
uniform vec3  uPrimaryColour;
uniform vec3  uSecondaryColour;
uniform vec3  uAccentColour;
uniform vec3  uBackground;
const float TAU=6.28318530718;
const vec3 NIGHT=vec3(0.006,0.010,0.035);
const vec3 BLUE=vec3(0.030,0.120,0.310);
const vec3 VIOLET=vec3(0.360,0.130,0.520);
const vec3 ROSE=vec3(0.920,0.360,0.560);
const vec3 GOLD=vec3(1.000,0.730,0.380);
vec3 themed(vec3 v, vec3 f){return dot(v,v)>0.0001?v:f;}
float hash21(vec2 p){p=fract(p*vec2(123.34,456.21));p+=dot(p,p+45.32);return fract(p.x*p.y);}
float noise(vec2 p){vec2 i=floor(p),f=fract(p),u=f*f*(3.0-2.0*f);float a=hash21(i),b=hash21(i+vec2(1,0)),c=hash21(i+vec2(0,1)),d=hash21(i+vec2(1,1));return mix(mix(a,b,u.x),mix(c,d,u.x),u.y);}
float fbm(vec2 p){float s=0.0,a=0.55;mat2 m=mat2(1.55,1.10,-1.10,1.55);for(int i=0;i<5;i++){s+=a*noise(p);p=m*p+0.17;a*=0.48;}return s;}
float sdSeg(vec2 p,vec2 a,vec2 b){vec2 pa=p-a,ba=b-a;float h=clamp(dot(pa,ba)/dot(ba,ba),0.0,1.0);return length(pa-ba*h);}
float line(float d,float w){return smoothstep(w,0.0,abs(d));}
float circle(vec2 p,vec2 c,float r,float w){return line(length(p-c)-r,w);}
vec3 grade(vec3 x){return clamp((x*(2.28*x+0.035))/(x*(2.10*x+0.56)+0.15),0.0,1.0);}
vec3 finish(vec2 uv, vec3 col, float glow){float r=length(uv);col+=col*col*glow;col+=GOLD*uBeat*uBeat*0.10;col*=1.0-0.46*smoothstep(0.78,1.75,r);col+=(hash21(gl_FragCoord.xy+floor(uTime*30.0))-0.5)*0.014;return grade(col*(1.04+uEnergy*0.20+uIntensity*0.10));}
// Crystal Goblet - rebuilt as a distinct TRANCE scene.
void main(){vec2 uv=vUV*2.0-1.0;uv.x*=uResolution.x/uResolution.y;float t=uTime*.11;vec3 col=mix(NIGHT,BLUE,.25+.15*uv.y);float bowl=line(abs(uv.x)-(.25+.08*uv.y),.018)*smoothstep(-.12,.62,uv.y)*smoothstep(.82,.45,uv.y);float stem=line(uv.x,.014)*smoothstep(-.82,-.12,uv.y);float base=line(uv.y+.82,.018)*smoothstep(.34,0.0,abs(uv.x));float refr=pow(abs(sin((uv.x*8.0+uv.y*5.0)+fbm(uv*4.0+t)*5.0)),16.0);col+=GOLD*(bowl+stem+base)*.65;col+=mix(ROSE,GOLD,refr)*refr*.28*smoothstep(.8,.0,abs(uv.x));fragColor=vec4(finish(uv,col,.62),1.0);}
