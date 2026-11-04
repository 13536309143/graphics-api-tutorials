#version 330 core

// 按实例数据放置粒子，并将朝向相机的四边形投影到屏幕。
layout(location = 0) in vec3 aPos;
layout(location = 2) in vec2 aUV;
layout(location = 3) in vec4 particle;
out vec2 uv;
out float fade;
uniform mat4 projection, view;
uniform vec3 right, up;
void main() {
    vec3 p = particle.xyz + (right * aPos.x + up * aPos.y) * .22;
    uv = aUV;
    fade = particle.w;
    gl_Position = projection * view * vec4(p, 1);
}
