#version 330 core
// 片段输入来自顶点插值；纹理单元、采样器与输出附件格式需和本章绑定对应。

// 以粒子纹理的透明度控制柔和边缘，配合 CPU 设置的混合状态。
in vec2 uv;
in float fade;
out vec4 FragColor;
void main() {
    float r = length(uv - .5) * 2.0;
    if (r >= 1.0)
        discard;
    float alpha = (1.0 - r) * (1.0 - fade);
    FragColor = vec4(mix(vec3(1, .7, .08), vec3(.9, .08, .01), fade), alpha);
}
