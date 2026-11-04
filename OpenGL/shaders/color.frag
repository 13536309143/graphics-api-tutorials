#version 330 core

// 把顶点阶段插值后的颜色写入当前颜色附件。
in vec3 color;
out vec4 FragColor;
uniform float pulse;
void main() {
    FragColor = vec4(color * pulse, 1.0);
}
