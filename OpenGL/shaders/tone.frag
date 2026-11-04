#version 330 core

// 在线性空间叠加 Bloom，再做 tone mapping 与显示 gamma 编码。
in vec2 uv;
out vec4 FragColor;
uniform sampler2D tex, bloomTex;
uniform bool toneMap, bloom;
uniform float exposure;
void main() {
    vec3 c = texture(tex, uv).rgb;
    if (bloom)
        c += texture(bloomTex, uv).rgb * .25;
    c = toneMap ? vec3(1) - exp(-c * exposure) : c;
    FragColor = vec4(pow(max(c, vec3(0)), vec3(1.0 / 2.2)), 1);
}
