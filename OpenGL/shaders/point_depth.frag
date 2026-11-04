#version 330 core
in vec3 worldPos;uniform vec3 light;uniform float farPlane;
void main(){gl_FragDepth=length(worldPos-light)/farPlane;}
