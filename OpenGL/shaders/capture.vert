#version 330 core
layout(location=0) in vec3 aPos;out vec3 direction;uniform mat4 projection,view;
void main(){direction=aPos;gl_Position=projection*view*vec4(aPos,1);}
