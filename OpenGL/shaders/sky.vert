#version 330 core

// 去掉视图平移，让天空随相机旋转并保持在远平面。
layout(location = 0) in vec3 aPos;
out vec3 direction;
uniform mat4 projection, view;
void main() {
    direction = aPos;
    vec4 p = projection * mat4(mat3(view)) * vec4(aPos, 1);
    gl_Position = p.xyww;
}
