#version 330 core

// 预计算 split-sum BRDF 的二维积分，运行时按 NoV 和粗糙度查表。
in vec2 uv;
out vec4 FragColor;

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

float G1(float x, float rough) {
    float k = rough * rough / 2.0;
    return x / (x * (1.0 - k) + k);
}
void main() {
    float NoV = max(uv.x, 0.001), rough = uv.y;
    vec3 V = vec3(sqrt(1.0 - NoV * NoV), 0, NoV), N = vec3(0, 0, 1);
    vec2 sum = vec2(0);
    for (uint i = 0u; i < 128u; ++i) {
        vec3 H = importanceGGX(hammersley(i), N, rough);
        vec3 L = normalize(2.0 * dot(V, H) * H - V);
        float NoL = max(L.z, 0.0), NoH = max(H.z, 0.0), VoH = max(dot(V, H), 0.0);
        if (NoL > 0.0) {
            float visibility = G1(NoV, rough) * G1(NoL, rough) * VoH / max(NoH * NoV, 0.001);
            float Fc = pow(1.0 - VoH, 5.0);
            sum += vec2((1.0 - Fc) * visibility, Fc * visibility);
        }
    }
    FragColor = vec4(sum / 128.0, 0, 1);
}
