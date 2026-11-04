#version 330 core

// 用六个朝向的视图矩阵将场景投影到 cubemap 各面。
layout(location = 0) in vec3 aPos;
out vec3 direction;
uniform mat4 projection, view;
void main() {
    direction = aPos;
    gl_Position = projection * view * vec4(aPos, 1);
}
