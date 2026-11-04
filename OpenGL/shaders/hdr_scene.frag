#version 330 core
in vec3 worldPos,normal;layout(location=0) out vec4 color;layout(location=1) out vec4 bright;
uniform vec3 baseColor,eye;uniform float emission;
void main(){vec3 N=normalize(normal),L=normalize(vec3(-3,4,5)-worldPos);vec3 c=baseColor*(0.05+max(dot(N,L),0.0)*3.0)+baseColor*emission;color=vec4(c,1);bright=vec4(dot(c,vec3(.2126,.7152,.0722))>1.0?c:vec3(0),1);}
