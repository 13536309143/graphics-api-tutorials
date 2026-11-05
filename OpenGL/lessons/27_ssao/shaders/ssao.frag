#version 330 core
// 片段输入来自顶点插值；纹理单元、采样器与输出附件格式需和本章绑定对应。

// 把半球样本投影回屏幕，与 G-buffer 深度比较以估计局部遮挡。
in vec2 uv;
out vec4 FragColor;
uniform sampler2D positionTex, normalTex, noiseTex;
uniform mat4 projection;
uniform vec3 samples[32];
void main() {
    vec4 p = texture(positionTex, uv);
    if (p.a < .5) {
        FragColor = vec4(1);
        return;
    }
    vec3 N = normalize(texture(normalTex, uv).xyz);
    vec2 noiseScale = vec2(textureSize(positionTex, 0)) / 4.0;
    vec3 random = texture(noiseTex, uv * noiseScale).xyz;
    vec3 T = normalize(random - N * dot(random, N));
    mat3 frame = mat3(T, cross(N, T), N);
    float occluded = 0.0;
    for (int i = 0; i < 32; ++i) {
        vec3 samplePos = p.xyz + frame * samples[i] * .8;
        vec4 clip = projection * vec4(samplePos, 1);
        vec2 tc = clip.xy / clip.w * .5 + .5;
        if (any(lessThan(tc, vec2(0))) || any(greaterThan(tc, vec2(1))))
            continue;
        vec4 neighbor = texture(positionTex, tc);
        if (neighbor.a < .5)
            continue;
        float range = smoothstep(0.0, 1.0, .8 / max(abs(p.z - neighbor.z), .001));
        occluded += (neighbor.z >= samplePos.z + .03 ? 1.0 : 0.0) * range;
    }
    FragColor = vec4(vec3(1.0 - occluded / 32.0), 1);
}
