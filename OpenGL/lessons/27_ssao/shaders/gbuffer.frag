#version 330 core
// 片段输入来自顶点插值；纹理单元、采样器与输出附件格式需和本章绑定对应。

// 将几何信息分别写入多个附件，供延迟光照与 SSAO 重用。
in vec3 worldPos, normal;
in vec2 uv;
layout(location = 0) out vec4 gPosition;
layout(location = 1) out vec4 gNormal;
layout(location = 2) out vec4 gAlbedo;
uniform mat4 view;
uniform vec3 baseColor;
void main() {
    gPosition = vec4((view * vec4(worldPos, 1)).xyz, 1);
    gNormal = vec4(normalize(mat3(view) * normal), 1);
    gAlbedo = vec4(baseColor, .5);
}
