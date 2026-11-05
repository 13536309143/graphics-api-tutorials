#version 330 core
// 片段输入来自顶点插值；纹理单元、采样器与输出附件格式需和本章绑定对应。

// 根据插值 UV 从指定 sampler 采样纹理。
in vec2 uv;
out vec4 FragColor;
uniform sampler2D tex;
void main() {
    FragColor = texture(tex, uv);
}
