#version 330 core

// 对离屏颜色纹理做全屏采样，并提供后处理切换。
in vec2 uv;
out vec4 FragColor;
uniform sampler2D tex;
uniform bool grayscale;
void main() {
    vec3 c = texture(tex, uv).rgb;
    if (grayscale)
        c = vec3(dot(c, vec3(0.2126, 0.7152, 0.0722)));
    FragColor = vec4(c, 1.0);
}
