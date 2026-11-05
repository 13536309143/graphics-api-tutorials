#version 330 core
// 片段输入来自顶点插值；纹理单元、采样器与输出附件格式需和本章绑定对应。

// 把三维方向转换为经纬 UV，采样 HDR 全景图。
in vec3 direction;
out vec4 FragColor;
uniform sampler2D panorama;
void main() {
    vec3 d = normalize(direction);
    vec2 uv = vec2(atan(d.z, d.x) / 6.2831853 + 0.5, acos(clamp(d.y, -1.0, 1.0)) / 3.14159265);
    FragColor = vec4(texture(panorama, uv).rgb, 1);
}
