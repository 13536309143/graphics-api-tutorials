#version 330 core
// 顶点输入位置与 VAO 属性对应；uniform 的名称和矩阵布局需与本章 C++ 设置一致。

// 使用实例属性组合模型变换，让一批顶点被重复放置。
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 3) in vec3 offset;
out vec3 color;
uniform mat4 vp;
uniform float time;
void main() {
    vec3 p = aPos * 0.35 + offset;
    p.y += sin(time + offset.x * 0.6 + offset.z * 0.3) * 0.5;
    gl_Position = vp * vec4(p, 1.0);
    color = (aNormal * 0.5 + 0.5) * 0.7 + 0.3;
}
