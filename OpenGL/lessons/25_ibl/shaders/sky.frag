#version 330 core
// 片段输入来自顶点插值；纹理单元、采样器与输出附件格式需和本章绑定对应。

// 按方向采样天空环境；显示编码由调用方决定。
in vec3 direction;
out vec4 FragColor;
uniform samplerCube environmentMap;
uniform bool displayEncode;
void main() {
    vec3 c = textureLod(environmentMap, direction, 0).rgb;
    if (displayEncode) {
        c = c / (c + vec3(1));
        c = pow(c, vec3(1.0 / 2.2));
    }
    FragColor = vec4(c, 1);
}
