#version 330 core
in vec3 worldPos,normal;out vec4 FragColor;uniform samplerCube depthCube;uniform vec3 light,baseColor,eye;uniform float farPlane;uniform bool shadows;
void main(){vec3 delta=worldPos-light;float stored=texture(depthCube,delta).r*farPlane;float current=length(delta);vec3 N=normalize(normal),L=normalize(-delta);float s=shadows && current-.05>stored?1.0:0.0;vec3 c=baseColor*(.1+(1.0-s)*max(dot(N,L),0.0)*4.0/(1.0+.15*current+.03*current*current));FragColor=vec4(pow(c,vec3(1.0/2.2)),1);}
