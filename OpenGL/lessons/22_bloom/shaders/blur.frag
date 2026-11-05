#version 330 core
// 片段输入来自顶点插值；纹理单元、采样器与输出附件格式需和本章绑定对应。

// 沿单一方向做高斯模糊，多次交替水平和垂直方向。
in vec2 uv;
out vec4 FragColor;
uniform sampler2D tex;
uniform bool horizontal;
void main() {
    vec2 stepUV = 1.0 / vec2(textureSize(tex, 0));
    float weights[5] = float[](.227027, .194595, .121622, .054054, .016216);
    vec3 c = texture(tex, uv).rgb * weights[0];
    for (int i = 1; i < 5; ++i) {
        vec2 d = horizontal ? vec2(stepUV.x * i, 0) : vec2(0, stepUV.y * i);
        c += (texture(tex, uv + d).rgb + texture(tex, uv - d).rgb) * weights[i];
    }
    FragColor = vec4(c, 1);
}
