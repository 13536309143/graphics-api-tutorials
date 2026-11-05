#version 330 core
// 片段输入来自顶点插值；纹理单元、采样器与输出附件格式需和本章绑定对应。

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
