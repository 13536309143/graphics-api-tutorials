#version 330 core
// 片段输入来自顶点插值；纹理单元、采样器与输出附件格式需和本章绑定对应。

// 在法线半球上积分环境辐射，得到漫反射辐照度。
in vec3 direction;
out vec4 FragColor;
uniform samplerCube environmentMap;

const float PI = 3.14159265359;
float radicalInverse(uint bits) {
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10;
}
vec2 hammersley(uint i) {
    return vec2(float(i) / 128.0, radicalInverse(i));
}
mat3 basis(vec3 N) {
    vec3 up = abs(N.z) < 0.999 ? vec3(0, 0, 1) : vec3(1, 0, 0);
    vec3 T = normalize(cross(up, N));
    return mat3(T, cross(N, T), N);
}
vec3 importanceGGX(vec2 Xi, vec3 N, float rough) {
    float a = rough * rough;
    float phi = 2.0 * PI * Xi.x;
    float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a * a - 1.0) * Xi.y));
    float sinTheta = sqrt(max(0.0, 1.0 - cosTheta * cosTheta));
    return normalize(basis(N) * vec3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta));
}

void main() {
    vec3 N = normalize(direction), sum = vec3(0);
    mat3 frame = basis(N);
    for (uint i = 0u; i < 128u; ++i) {
        vec2 Xi = hammersley(i);
        float phi = 2.0 * PI * Xi.x;
        float r = sqrt(Xi.y);
        vec3 L = frame * vec3(r * cos(phi), r * sin(phi), sqrt(1.0 - Xi.y));
        sum += textureLod(environmentMap, L, 0).rgb;
    }
    FragColor = vec4(PI * sum / 128.0, 1);
}
