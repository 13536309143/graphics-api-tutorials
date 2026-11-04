#version 330 core

// 保存到光源的径向距离，供深度 cubemap 全向阴影比较。
in vec3 worldPos;
uniform vec3 light;
uniform float farPlane;
void main() {
    gl_FragDepth = length(worldPos - light) / farPlane;
}
