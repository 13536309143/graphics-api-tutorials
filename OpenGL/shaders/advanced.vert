#version 330 core

// 把位置、法线和 UV 变换到后续材质计算需要的空间。
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aUV;
out vec3 worldPos, normal;
out vec2 uv;
uniform mat4 model, view, projection;
void main() {
    vec4 p = model * vec4(aPos, 1);
    worldPos = p.xyz;
    normal = mat3(transpose(inverse(model))) * aNormal;
    uv = aUV;
    gl_Position = projection * view * p;
}
