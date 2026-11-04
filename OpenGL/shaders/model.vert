#version 330 core
layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aNormal;
layout(location=2) in vec2 aUV;
layout(location=3) in ivec4 aJoints;
layout(location=4) in vec4 aWeights;
out vec3 worldPos,normal;
out vec2 uv;
uniform mat4 model,view,projection,bones[48];
uniform bool skinned;
void main(){
    mat4 skin=mat4(1);
    if(skinned && dot(aWeights,vec4(1))>0.0)skin=bones[aJoints.x]*aWeights.x+bones[aJoints.y]*aWeights.y+bones[aJoints.z]*aWeights.z+bones[aJoints.w]*aWeights.w;
    vec4 p=model*skin*vec4(aPos,1);worldPos=p.xyz;
    normal=mat3(transpose(inverse(model*skin)))*aNormal;uv=aUV;gl_Position=projection*view*p;
}
