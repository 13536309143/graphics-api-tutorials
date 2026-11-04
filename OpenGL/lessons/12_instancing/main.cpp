#include "app.hpp"

// 实例化绘制：配合本目录 README.md 阅读。
// 本课重点：为实例属性设置 divisor，一次提交绘制多个对象。
int main(int argc, char** argv) try {
    App app(argc, argv, "12_instancing - OpenGL Course");
    Shader shader(app, "instance.vert", "color.frag");
    Mesh mesh(cube());
    Camera camera;
    camera.pos = glm::vec3(0, 5, 18);
    camera.pitch = -15;
    std::vector<glm::vec3> offsets;
    for (int z = 0; z < 30; ++z)
        for (int x = 0; x < 30; ++x)
            offsets.emplace_back((x - 15) * .8f, 0, -z * .8f);
    GLuint instances;
    glGenBuffers(1, &instances);
    glBindVertexArray(mesh.vao);
    glBindBuffer(GL_ARRAY_BUFFER, instances);
    glBufferData(GL_ARRAY_BUFFER,
                 GLsizeiptr(offsets.size() * sizeof(glm::vec3)),
                 offsets.data(),
                 GL_STATIC_DRAW);
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), nullptr);
    glVertexAttribDivisor(3, 1); // 每个实例读一个 offset，而不是每个顶点读一个。
    glEnable(GL_DEPTH_TEST);
    while (app.begin()) {
        camera.update(app);
        app.clear();
        shader.use();
        shader.set("vp", camera.projection(app) * camera.view());
        shader.set("time", app.time());
        shader.set("pulse", 1.f);
        glBindVertexArray(mesh.vao);
        glDrawArraysInstanced(GL_TRIANGLES, 0, mesh.count, GLsizei(offsets.size()));
        app.end();
    }
    glDeleteBuffers(1, &instances);
    return 0;
} catch (const std::exception& e) {
    return failure(e);
}
