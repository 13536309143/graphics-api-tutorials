#version 330 core
// 片段输入来自顶点插值；纹理单元、采样器与输出附件格式需和本章绑定对应。

// 保存到光源的径向距离，供深度 cubemap 全向阴影比较。
in vec3 worldPos;
uniform vec3 light;
uniform float farPlane;
void main() {
    gl_FragDepth = length(worldPos - light) / farPlane;
}
