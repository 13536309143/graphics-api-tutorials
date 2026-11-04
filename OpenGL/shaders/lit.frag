#version 330 core

// 在同一空间计算光照；阴影比较时加入偏移以降低自遮挡条纹。
in vec3 worldPos, normal;
in vec2 uv;
in vec4 lightPos;
out vec4 FragColor;
uniform vec3 eye, light, baseColor;
uniform sampler2D tex;
uniform sampler2D shadowMap;
uniform bool textured, multi, shadows;
uniform float shininess;
float shadowFactor(vec3 N, vec3 L) {
    vec3 p = lightPos.xyz / lightPos.w;
    p = p * 0.5 + 0.5;
    if (p.z > 1.0 || p.z < 0.0)
        return 0.0;
    float bias = max(0.005 * (1.0 - dot(N, L)), 0.001);
    vec2 stepUV = 1.0 / vec2(textureSize(shadowMap, 0));
    float s = 0.0;
    for (int x = -1; x <= 1; ++x)
        for (int y = -1; y <= 1; ++y)
            s += p.z - bias > texture(shadowMap, p.xy + vec2(x, y) * stepUV).r ? 1.0 : 0.0;
    return s / 9.0;
}
vec3 phong(vec3 lp, vec3 tint, vec3 albedo, vec3 N, vec3 V, float shadow) {
    vec3 L = normalize(lp - worldPos);
    float diffuse = max(dot(N, L), 0.0);
    float spec = diffuse > 0.0 ? pow(max(dot(V, reflect(-L, N)), 0.0), shininess) : 0.0;
    return (1.0 - shadow) * (diffuse * albedo + 0.45 * spec) * tint;
}
void main() {
    vec3 N = normalize(normal), V = normalize(eye - worldPos);
    vec3 albedo = baseColor * (textured ? texture(tex, uv).rgb : vec3(1.0));
    float s = shadows ? shadowFactor(N, normalize(light - worldPos)) : 0.0;
    vec3 result = 0.12 * albedo + phong(light, vec3(1.0), albedo, N, V, s);
    if (multi) {
        vec3 lp = vec3(-3.0, 1.0, 2.0);
        float d = length(lp - worldPos);
        result +=
            phong(lp, vec3(0.2, 0.5, 1.0), albedo, N, V, 0.0) / (1.0 + 0.09 * d + 0.032 * d * d);
    }
    FragColor = vec4(result, 1.0);
}
