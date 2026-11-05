#version 330 core
// 片段输入来自顶点插值；纹理单元、采样器与输出附件格式需和本章绑定对应。

// 由视线和法线计算反射方向，再采样环境 cubemap。
in vec3 worldPos, normal;
out vec4 FragColor;
uniform vec3 eye;
uniform samplerCube environmentMap;
void main() {
    vec3 R = reflect(normalize(worldPos - eye), normalize(normal));
    vec3 c = texture(environmentMap, R).rgb;
    c = c / (c + vec3(1));
    FragColor = vec4(pow(c, vec3(1.0 / 2.2)), 1);
}
