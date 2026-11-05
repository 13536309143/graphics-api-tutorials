#version 450
// 本章顶点阶段：核对输入属性位置/语义、矩阵内存布局以及最终裁剪空间位置。
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aUV;

// binding 0 是每帧矩阵；push constants 是每次绘制的模型矩阵与材质参数。
layout(binding = 0) uniform Frame {
    mat4 viewProjection;
}
frame;
layout(push_constant) uniform Draw {
    mat4 model;
    vec4 settings;
}
draw;
layout(location = 0) out vec3 normal;
layout(location = 1) out vec2 uv;
layout(location = 2) out vec3 worldPos;
void main() {
    int mode = int(draw.settings.x);
    vec3 p = aPos, n = aNormal;
    uv = aUV;
    if (mode == 3) {
        vec2 points[3] = vec2[](vec2(-.7, .6), vec2(.7, .6), vec2(0, -.6));
        p = vec3(points[gl_VertexIndex], 0);
        n = vec3(gl_VertexIndex == 0, gl_VertexIndex == 1, gl_VertexIndex == 2);
        uv = vec2(0);
    }

    // 实例索引给每个立方体不同偏移，不需要重复上传顶点。
    if (mode == 7)
        p += vec3(float(gl_InstanceIndex % 5 - 2) * 1.5, 0, -float(gl_InstanceIndex / 5) * 1.5);

    // 法线使用模型矩阵的逆转置；这样非均匀缩放时仍保持与表面垂直。
    vec4 world = draw.model * vec4(p, 1);
    worldPos = world.xyz;
    normal = mat3(transpose(inverse(draw.model))) * n;
    gl_Position = frame.viewProjection * world;
}
