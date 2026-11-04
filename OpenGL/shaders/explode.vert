#version 330 core

// 向几何阶段传递位置数据，供逐三角形的效果使用。
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
out vec3 vertexNormal;
void main() {
    gl_Position = vec4(aPos, 1);
    vertexNormal = aNormal;
}
