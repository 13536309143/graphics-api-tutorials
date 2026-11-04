#version 330 core

// 从光源视角投影几何，生成阴影比较所需的深度。
layout(location = 0) in vec3 aPos;
uniform mat4 lightSpace, model;
void main() {
    gl_Position = lightSpace * model * vec4(aPos, 1.0);
}
