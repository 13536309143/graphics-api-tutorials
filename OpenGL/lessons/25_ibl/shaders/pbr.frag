#version 330 core
// 片段输入来自顶点插值；纹理单元、采样器与输出附件格式需和本章绑定对应。

// 在线性空间组合直接光与可选 IBL，最后根据显示路径决定是否编码。
in vec3 worldPos, normal;
in vec2 uv;
out vec4 FragColor;
uniform vec3 eye, baseColor;
uniform float metallic, roughness;
uniform bool useMaps, useNormal, useIBL, displayEncode;
uniform sampler2D albedoMap, normalMap, mrMap, aoMap, brdfLUT;
uniform samplerCube irradianceMap, prefilterMap;

mat3 tangentFrame(vec3 N) {
    vec3 q1 = dFdx(worldPos), q2 = dFdy(worldPos);
    vec2 t1 = dFdx(uv), t2 = dFdy(uv);
    float det = t1.x * t2.y - t1.y * t2.x;
    if (abs(det) < 0.000001) {
        vec3 T = normalize(cross(abs(N.y) < 0.99 ? vec3(0, 1, 0) : vec3(1, 0, 0), N));
        return mat3(T, cross(N, T), N);
    }
    vec3 T = normalize((q1 * t2.y - q2 * t1.y) / det);
    T = normalize(T - N * dot(N, T));
    vec3 rawB = (q2 * t1.x - q1 * t2.x) / det;
    vec3 B = normalize(cross(N, T)) * sign(dot(cross(N, T), rawB));
    return mat3(T, B, N);
}

const float PI = 3.14159265359;

// D：GGX 法线分布；G：几何遮蔽；F：Schlick 菲涅耳。分母下限用于数值稳定。
float distributionGGX(float NoH, float r) {
    float a = r * r, a2 = a * a;
    float d = NoH * NoH * (a2 - 1.0) + 1.0;
    return a2 / max(PI * d * d, 0.000001);
}
float geometrySchlick(float NoV, float r) {
    float k = (r + 1.0) * (r + 1.0) / 8.0;
    return NoV / (NoV * (1.0 - k) + k);
}
vec3 fresnel(float VoH, vec3 f0) {
    return f0 + (1.0 - f0) * pow(clamp(1.0 - VoH, 0.0, 1.0), 5.0);
}
vec3 directBRDF(vec3 N, vec3 V, vec3 L, vec3 albedo, float metal, float rough, vec3 radiance) {
    vec3 H = normalize(V + L);
    float NoV = max(dot(N, V), 0.0001), NoL = max(dot(N, L), 0.0);
    vec3 F = fresnel(max(dot(V, H), 0.0), mix(vec3(0.04), albedo, metal));
    float D = distributionGGX(max(dot(N, H), 0.0), rough),
          G = geometrySchlick(NoV, rough) * geometrySchlick(NoL, rough);
    vec3 spec = D * G * F / max(4.0 * NoV * NoL, 0.0001);
    vec3 kd = (1.0 - F) * (1.0 - metal);
    return (kd * albedo / PI + spec) * radiance * NoL;
}

void main() {
    vec3 N = normalize(normal), V = normalize(eye - worldPos), albedo = baseColor;
    float metal = metallic, rough = roughness, ao = 1.0;

    // glTF 金属粗糙度贴图用 G 通道存粗糙度、B 通道存金属度，AO 单独采样。
    if (useMaps) {
        albedo *= texture(albedoMap, uv).rgb;
        vec3 mr = texture(mrMap, uv).rgb;
        metal *= mr.b;
        rough *= mr.g;
        ao = texture(aoMap, uv).r;
    }
    if (useNormal)
        N = normalize(tangentFrame(N) * (texture(normalMap, uv).xyz * 2.0 - 1.0));
    rough = clamp(rough, 0.045, 1.0);
    metal = clamp(metal, 0.0, 1.0);
    vec3 positions[4] = vec3[](vec3(-6, 6, 8), vec3(6, 6, 8), vec3(-6, -4, 6), vec3(6, -4, 6));
    vec3 result = vec3(0);
    for (int i = 0; i < 4; ++i) {
        vec3 d = positions[i] - worldPos;
        result += directBRDF(N, V, normalize(d), albedo, metal, rough, vec3(90) / dot(d, d));
    }

    // split-sum IBL：漫反射查辐照度，镜面查预过滤环境图并乘 BRDF LUT。
    if (useIBL) {
        float NoV = max(dot(N, V), 0.0);
        vec3 f0 = mix(vec3(0.04), albedo, metal);
        vec3 F = f0 + (max(vec3(1.0 - rough), f0) - f0) * pow(1.0 - NoV, 5.0);
        vec3 diffuse = texture(irradianceMap, N).rgb * albedo / PI;
        vec3 spec = textureLod(prefilterMap, reflect(-V, N), rough * 4.0).rgb;
        vec2 envBRDF = texture(brdfLUT, vec2(NoV, rough)).rg;
        result += ((1.0 - F) * (1.0 - metal) * diffuse + spec * (f0 * envBRDF.x + envBRDF.y)) * ao;
    } else
        result += 0.025 * albedo * ao * (1.0 - metal);

    // 只在直接显示时做 tone mapping/gamma；写入 HDR 中间目标时保留线性颜色。
    if (displayEncode) {
        result = result / (result + vec3(1));
        result = pow(max(result, vec3(0)), vec3(1.0 / 2.2));
    }
    FragColor = vec4(result, 1);
}
