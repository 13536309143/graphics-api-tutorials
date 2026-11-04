#version 330 core

// 根据插值 UV 从指定 sampler 采样纹理。
in vec2 uv;
out vec4 FragColor;
uniform sampler2D tex;
void main() {
    FragColor = texture(tex, uv);
}
