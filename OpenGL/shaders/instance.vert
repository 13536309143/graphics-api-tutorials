#version 330 core
layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aNormal;
layout(location=3) in vec3 offset;
out vec3 color;
uniform mat4 vp;
uniform float time;
void main(){
    vec3 p=aPos*0.35+offset;
    p.y+=sin(time+offset.x*0.6+offset.z*0.3)*0.5;
    gl_Position=vp*vec4(p,1.0);
    color=(aNormal*0.5+0.5)*0.7+0.3;
}
