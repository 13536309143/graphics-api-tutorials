// 12_instancing
// 本目录拥有完整课程源码；只共享第三方依赖，不链接其他章节的课程实现。
#include "support/app.hpp"

// 实例化绘制：配合本目录 README.md 阅读。
// 本课重点：为实例属性设置 divisor，一次提交绘制多个对象。
int main(int argc, char** argv) try {
    // 先创建上下文；它必须比 shader、纹理和网格活得更久。
    App app(argc, argv, "12_instancing - OpenGL Course");
    // shader 路径指向本课自己的文件；属性、uniform 与下方 C++ 设置对应。
    Shader shader(app, "instance.vert", "color.frag");
    // CPU 顶点数据上传后，由 VAO 保存属性布局；布局必须匹配顶点 shader。
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
    // 每帧处理输入并绘制；GPU 对象在退出循环后按 C++ 作用域逆序释放。
    while (app.begin()) {
        camera.update(app);
        app.clear();
        shader.use();
        shader.set("vp", camera.projection(app) * camera.view());
        shader.set("time", app.time());
        shader.set("pulse", 1.f);
        glBindVertexArray(mesh.vao);
        // 绘制使用当前程序、VAO 和附件；属性布局必须匹配顶点着色器。
        glDrawArraysInstanced(GL_TRIANGLES, 0, mesh.count, GLsizei(offsets.size()));
        app.end();
    }
    glDeleteBuffers(1, &instances);
    return 0;
} catch (const std::exception& e) {
    return failure(e);
}
