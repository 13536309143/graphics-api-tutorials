#version 330 core
// 片段输入来自顶点插值；纹理单元、采样器与输出附件格式需和本章绑定对应。

// 按光源方向采样深度 cubemap，并把比较结果用于光照衰减。
in vec3 worldPos, normal;
out vec4 FragColor;
uniform samplerCube depthCube;
uniform vec3 light, baseColor, eye;
uniform float farPlane;
uniform bool shadows;
void main() {
    vec3 delta = worldPos - light;
    float stored = texture(depthCube, delta).r * farPlane;
    float current = length(delta);
    vec3 N = normalize(normal), L = normalize(-delta);
    float s = shadows && current - .05 > stored ? 1.0 : 0.0;
    vec3 c = baseColor * (.1 + (1.0 - s) * max(dot(N, L), 0.0) * 4.0 /
                                   (1.0 + .15 * current + .03 * current * current));
    FragColor = vec4(pow(c, vec3(1.0 / 2.2)), 1);
}
