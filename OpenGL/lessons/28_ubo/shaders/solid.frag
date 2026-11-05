#version 330 core
// 片段输入来自顶点插值；纹理单元、采样器与输出附件格式需和本章绑定对应。

// 输出固定颜色，供 stencil 描边等只关心覆盖范围的通道使用。
out vec4 FragColor;
uniform vec3 baseColor;
uniform float opacity;
void main() {
    FragColor = vec4(baseColor, opacity);
}
