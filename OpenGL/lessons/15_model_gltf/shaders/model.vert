#version 330 core
// 顶点输入位置与 VAO 属性对应；uniform 的名称和矩阵布局需与本章 C++ 设置一致。

// 按骨骼索引和权重混合蒙皮矩阵，再应用模型/相机变换。
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aUV;
layout(location = 3) in ivec4 aJoints;
layout(location = 4) in vec4 aWeights;
out vec3 worldPos, normal;
out vec2 uv;
uniform mat4 model, view, projection, bones[48];
uniform bool skinned;
void main() {
    mat4 skin = mat4(1);
    if (skinned && dot(aWeights, vec4(1)) > 0.0)
        skin = bones[aJoints.x] * aWeights.x + bones[aJoints.y] * aWeights.y +
               bones[aJoints.z] * aWeights.z + bones[aJoints.w] * aWeights.w;
    vec4 p = model * skin * vec4(aPos, 1);
    worldPos = p.xyz;
    normal = mat3(transpose(inverse(model * skin))) * aNormal;
    uv = aUV;
    gl_Position = projection * view * p;
}
