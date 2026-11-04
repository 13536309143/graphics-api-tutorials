#version 330 core
in vec3 worldPos,normal;out vec4 FragColor;uniform vec3 eye;uniform samplerCube environmentMap;
void main(){vec3 R=reflect(normalize(worldPos-eye),normalize(normal));vec3 c=texture(environmentMap,R).rgb;c=c/(c+vec3(1));FragColor=vec4(pow(c,vec3(1.0/2.2)),1);}
