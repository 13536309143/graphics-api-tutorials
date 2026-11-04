#version 330 core

// 按固定 attribute 布局读取位置与颜色，并输出插值数据。
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aColor;
layout(location = 2) in vec2 aUV;
out vec3 color;
out vec2 uv;
uniform mat4 mvp;
void main() {
    gl_Position = mvp * vec4(aPos, 1.0);
    color = aColor;
    uv = aUV;
}
