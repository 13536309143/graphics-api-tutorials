#version 330 core
// 片段输入来自顶点插值；纹理单元、采样器与输出附件格式需和本章绑定对应。

// 把顶点阶段插值后的颜色写入当前颜色附件。
in vec3 color;
out vec4 FragColor;
uniform float pulse;
void main() {
    FragColor = vec4(color * pulse, 1.0);
}
