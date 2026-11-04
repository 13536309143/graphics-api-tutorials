#version 330 core

// 读取 G-buffer 后统一计算屏幕空间光照，并可调试中间附件。
in vec2 uv;
out vec4 FragColor;
uniform sampler2D positionTex, normalTex, albedoTex, aoTex;
uniform mat4 view;
uniform bool useAO;
uniform int debugView;
void main() {
    vec4 p = texture(positionTex, uv);
    if (p.a < .5) {
        FragColor = vec4(.02, .025, .04, 1);
        return;
    }
    vec3 N = normalize(texture(normalTex, uv).xyz), albedo = texture(albedoTex, uv).rgb;
    float ao = useAO ? texture(aoTex, uv).r : 1.0;
    if (debugView == 1) {
        FragColor = vec4(vec3(ao), 1);
        return;
    }
    if (debugView == 2) {
        FragColor = vec4(N * .5 + .5, 1);
        return;
    }
    vec3 color = .15 * albedo * ao, V = normalize(-p.xyz);
    for (int i = 0; i < 32; ++i) {
        float t = float(i);
        vec3 worldLight = vec3(sin(t * 2.4) * 5.0, 1.0 + mod(t, 3.0), cos(t * 1.7) * 5.0);
        vec3 d = (view * vec4(worldLight, 1)).xyz - p.xyz;
        float distance = length(d);
        vec3 L = d / distance;
        vec3 tint = .5 + .5 * cos(vec3(0, 2, 4) + t);
        vec3 H = normalize(V + L);
        color += (albedo * max(dot(N, L), 0.0) + pow(max(dot(N, H), 0.0), 32.0) * .25) * tint *
                 2.0 / (1.0 + .2 * distance + .12 * distance * distance);
    }
    color = color / (color + vec3(1));
    FragColor = vec4(pow(color, vec3(1.0 / 2.2)), 1);
}
