#version 330 core

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
