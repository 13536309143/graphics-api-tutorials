#version 330 core
layout(location=0) in vec3 aPos;out vec3 worldPos;uniform mat4 model,lightVP;
void main(){vec4 p=model*vec4(aPos,1);worldPos=p.xyz;gl_Position=lightVP*p;}
