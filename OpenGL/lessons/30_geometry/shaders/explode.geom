#version 330 core
// 几何阶段接收完整图元；输出顶点数量不得超过 max_vertices，EmitVertex 后结束图元。
layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;
in vec3 vertexNormal[];
out vec3 worldPos, normal;
out vec2 uv;
uniform mat4 model, view, projection;
uniform float time;
void main() {
    vec3 a = gl_in[1].gl_Position.xyz - gl_in[0].gl_Position.xyz,
         b = gl_in[2].gl_Position.xyz - gl_in[0].gl_Position.xyz;
    vec3 face = normalize(cross(a, b));
    float amount = (sin(time) + 1.0) * .25;
    for (int i = 0; i < 3; ++i) {
        vec4 p = model * (gl_in[i].gl_Position + vec4(face * amount, 0));
        worldPos = p.xyz;
        normal = mat3(transpose(inverse(model))) * vertexNormal[i];
        uv = vec2(0);
        gl_Position = projection * view * p;
        EmitVertex();
    }
    EndPrimitive();
}
