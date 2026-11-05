#version 330 core
// 顶点输入位置与 VAO 属性对应；uniform 的名称和矩阵布局需与本章 C++ 设置一致。

// 按当前 cubemap 面的光源矩阵投影顶点。
layout(location = 0) in vec3 aPos;
out vec3 worldPos;
uniform mat4 model, lightVP;
void main() {
    vec4 p = model * vec4(aPos, 1);
    worldPos = p.xyz;
    gl_Position = lightVP * p;
}
