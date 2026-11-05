#version 330 core
// 片段输入来自顶点插值；纹理单元、采样器与输出附件格式需和本章绑定对应。

// 在线性空间输出可超过 1 的高亮颜色，交给浮点渲染目标保存。
in vec3 worldPos, normal;
layout(location = 0) out vec4 color;
layout(location = 1) out vec4 bright;
uniform vec3 baseColor, eye;
uniform float emission;
void main() {
    vec3 N = normalize(normal), L = normalize(vec3(-3, 4, 5) - worldPos);
    vec3 c = baseColor * (0.05 + max(dot(N, L), 0.0) * 3.0) + baseColor * emission;
    color = vec4(c, 1);
    bright = vec4(dot(c, vec3(.2126, .7152, .0722)) > 1.0 ? c : vec3(0), 1);
}
