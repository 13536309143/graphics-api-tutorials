#version 330 core
// 顶点输入位置与 VAO 属性对应；uniform 的名称和矩阵布局需与本章 C++ 设置一致。

// 去掉视图平移，让天空随相机旋转并保持在远平面。
layout(location = 0) in vec3 aPos;
out vec3 direction;
uniform mat4 projection, view;
void main() {
    direction = aPos;
    vec4 p = projection * mat4(mat3(view)) * vec4(aPos, 1);
    gl_Position = p.xyww;
}
