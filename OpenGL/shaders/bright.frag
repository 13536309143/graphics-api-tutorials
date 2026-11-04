#version 330 core

// 从线性 HDR 颜色提取高亮部分，作为 Bloom 的输入。
in vec2 uv;
out vec4 FragColor;
uniform sampler2D tex;
void main() {
    vec3 c = texture(tex, uv).rgb;
    FragColor = vec4(dot(c, vec3(.2126, .7152, .0722)) > 1.0 ? c : vec3(0), 1);
}
