#version 330 core

// 输出固定颜色，供 stencil 描边等只关心覆盖范围的通道使用。
out vec4 FragColor;
uniform vec3 baseColor;
uniform float opacity;
void main() {
    FragColor = vec4(baseColor, opacity);
}
