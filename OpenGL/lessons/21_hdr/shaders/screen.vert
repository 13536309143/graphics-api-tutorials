#version 330 core
// 顶点输入位置与 VAO 属性对应；uniform 的名称和矩阵布局需与本章 C++ 设置一致。

// 全屏四边形直接输出裁剪空间位置及纹理坐标。
layout(location = 0) in vec3 aPos;
layout(location = 2) in vec2 aUV;
out vec2 uv;
void main() {
    uv = aUV;
    gl_Position = vec4(aPos.xy / 0.65, 0.0, 1.0);
}
