#version 450
// 本章片段阶段：输入来自顶点插值，纹理绑定和输出颜色格式需与 C++ 管线一致。
layout(location = 0) in vec3 normal;
layout(location = 1) in vec2 uv;
layout(location = 2) in vec3 worldPos;
layout(binding = 1) uniform sampler2D tex;
layout(push_constant) uniform Draw {
    mat4 model;
    vec4 settings;
}
draw;
layout(location = 0) out vec4 color;

const float PI = 3.14159265359;

// 直接光 Cook-Torrance BRDF：D 用 GGX，G 用 Schlick-GGX，F 用 Schlick 近似。
// metal 决定漫反射与镜面反射比例，r 为粗糙度；本路径不包含完整 IBL。
vec3 pbr(vec3 N, vec3 V, vec3 P, float metal, float r) {
    vec3 albedo = vec3(.8, .25, .07), sum = vec3(.02) * albedo * (1.0 - metal),
         f0 = mix(vec3(.04), albedo, metal);
    for (int i = 0; i < 4; ++i) {
        vec3 lp = vec3(i % 2 == 0 ? -6 : 6, i < 2 ? 6 : -4, 8);
        vec3 delta = lp - P, L = normalize(delta), H = normalize(V + L);
        float NoV = max(dot(N, V), .001), NoL = max(dot(N, L), 0), NoH = max(dot(N, H), 0);
        float a = r * r, a2 = a * a, d = NoH * NoH * (a2 - 1) + 1,
              D = a2 / max(PI * d * d, .000001);
        float k = (r + 1) * (r + 1) / 8;
        float G = NoV / (NoV * (1 - k) + k) * NoL / (NoL * (1 - k) + k);
        vec3 F = f0 + (1 - f0) * pow(1 - max(dot(V, H), 0), 5);
        sum += ((1 - F) * (1 - metal) * albedo / PI + D * G * F / max(4 * NoV * NoL, .001)) * 90 /
               dot(delta, delta) * NoL;
    }
    return sum;
}

void main() {
    int mode = int(draw.settings.x);
    vec3 N = normalize(normal), c;
    if (mode <= 4)
        c = clamp(normal, 0, 1);
    else if (mode >= 9)
        c = pbr(
            N, normalize(vec3(0, 0, 14) - worldPos), worldPos, draw.settings.y, draw.settings.z);
    else {
        c = mode >= 6 ? texture(tex, uv).rgb : vec3(.25, .6, .9);
        c *= .15 + max(dot(N, normalize(vec3(-3, 5, 4) - worldPos)), 0);
    }

    // 先做简单 tone mapping，再编码到显示用颜色范围。
    if (mode >= 9)
        c = c / (1 + c);
    color = vec4(pow(max(c, vec3(0)), vec3(1.0 / 2.2)), 1);
}
