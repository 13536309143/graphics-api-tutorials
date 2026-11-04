#version 330 core

// 结合位置和法线相似度降噪，避免把遮挡跨几何边缘混合。
in vec2 uv;
out vec4 FragColor;
uniform sampler2D tex, positionTex, normalTex;
void main() {
    vec2 stepUV = 1.0 / vec2(textureSize(tex, 0));
    vec3 N = texture(normalTex, uv).xyz;
    float z = texture(positionTex, uv).z;
    float sum = 0.0, total = 0.0;
    for (int y = -2; y <= 2; ++y)
        for (int x = -2; x <= 2; ++x) {
            vec2 tc = uv + vec2(x, y) * stepUV;
            float w = exp(-float(x * x + y * y) / 4.0) *
                      exp(-abs(texture(positionTex, tc).z - z) * 10.0) *
                      pow(max(dot(N, texture(normalTex, tc).xyz), 0.0), 8.0);
            sum += texture(tex, tc).r * w;
            total += w;
        }
    FragColor = vec4(vec3(total > .001 ? sum / total : 1.0), 1);
}
