#version 330 core

// 从共享 uniform block 读取相机矩阵，布局需与 CPU 端一致。
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(std140) uniform CameraBlock {
    mat4 projection;
    mat4 view;
};
uniform mat4 model;
out vec3 worldPos, normal;
void main() {
    vec4 p = model * vec4(aPos, 1);
    worldPos = p.xyz;
    normal = mat3(transpose(inverse(model))) * aNormal;
    gl_Position = projection * view * p;
}
