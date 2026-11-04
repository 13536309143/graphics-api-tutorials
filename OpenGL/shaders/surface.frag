#version 330 core
in vec3 worldPos,normal;in vec2 uv;out vec4 FragColor;
uniform vec3 baseColor,light,eye;
uniform float opacity,heightScale;
uniform bool useTexture,useNormal,useParallax;
uniform sampler2D albedoMap,normalMap,heightMap;

mat3 tangentFrame(vec3 N){
    vec3 q1=dFdx(worldPos),q2=dFdy(worldPos);vec2 t1=dFdx(uv),t2=dFdy(uv);
    float det=t1.x*t2.y-t1.y*t2.x;
    if(abs(det)<0.000001){vec3 T=normalize(cross(abs(N.y)<0.99?vec3(0,1,0):vec3(1,0,0),N));return mat3(T,cross(N,T),N);}
    vec3 T=normalize((q1*t2.y-q2*t1.y)/det);T=normalize(T-N*dot(N,T));vec3 rawB=(q2*t1.x-q1*t2.x)/det;vec3 B=normalize(cross(N,T))*sign(dot(cross(N,T),rawB));return mat3(T,B,N);
}

vec2 parallax(vec2 coord,vec3 V){
    float layers=mix(40.0,12.0,abs(V.z));float layerDepth=1.0/layers;
    vec2 delta=V.xy/max(abs(V.z),0.1)*heightScale/layers;
    vec2 current=coord;float depth=0.0;float sampled=1.0-texture(heightMap,current).r;
    for(int i=0;i<40;++i){if(depth>=sampled)break;current-=delta;depth+=layerDepth;sampled=1.0-texture(heightMap,current).r;}
    vec2 previous=current+delta;float after=sampled-depth;float before=1.0-texture(heightMap,previous).r-depth+layerDepth;
    float weight=after/(after-before);return mix(current,previous,clamp(weight,0.0,1.0));
}
void main(){
    vec3 N=normalize(normal),V=normalize(eye-worldPos);mat3 TBN=tangentFrame(N);vec2 tc=uv;
    if(useParallax){tc=parallax(uv,transpose(TBN)*V);if(any(lessThan(tc,vec2(0)))||any(greaterThan(tc,vec2(1))))discard;}
    vec4 albedo=useTexture?texture(albedoMap,tc):vec4(1);
    if(albedo.a<0.03)discard;
    if(useNormal)N=normalize(TBN*(texture(normalMap,tc).xyz*2.0-1.0));
    vec3 L=normalize(light-worldPos),H=normalize(L+V);
    vec3 c=baseColor*albedo.rgb*(0.12+max(dot(N,L),0.0))+vec3(0.35)*pow(max(dot(N,H),0.0),48.0);
    FragColor=vec4(pow(max(c,vec3(0)),vec3(1.0/2.2)),opacity*albedo.a);
}
