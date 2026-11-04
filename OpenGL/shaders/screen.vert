#version 330 core

// 全屏四边形直接输出裁剪空间位置及纹理坐标。
layout(location = 0) in vec3 aPos;
layout(location = 2) in vec2 aUV;
out vec2 uv;
void main() {
    uv = aUV;
    gl_Position = vec4(aPos.xy / 0.65, 0.0, 1.0);
}
