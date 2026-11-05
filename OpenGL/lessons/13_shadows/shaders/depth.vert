#version 330 core
// 顶点输入位置与 VAO 属性对应；uniform 的名称和矩阵布局需与本章 C++ 设置一致。

// 从光源视角投影几何，生成阴影比较所需的深度。
layout(location = 0) in vec3 aPos;
uniform mat4 lightSpace, model;
void main() {
    gl_Position = lightSpace * model * vec4(aPos, 1.0);
}
