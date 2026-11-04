#version 330 core
layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aNormal;
layout(location=2) in vec2 aUV;
out vec3 worldPos;
out vec3 normal;
out vec2 uv;
out vec4 lightPos;
uniform mat4 model,view,projection,lightSpace;
void main(){
    vec4 world=model*vec4(aPos,1.0);
    worldPos=world.xyz;
    normal=mat3(transpose(inverse(model)))*aNormal;
    uv=aUV;
    lightPos=lightSpace*world;
    gl_Position=projection*view*world;
}
