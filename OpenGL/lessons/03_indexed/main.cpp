#include "app.hpp"

// 索引绘制与线框：配合本目录 README.md 阅读。
// 本课重点：用 EBO 复用四边形顶点，比较索引数和顶点数。
int main(int argc, char** argv) try {
    App app(argc, argv, "03_indexed - OpenGL Course");
    Shader shader(app, "basic.vert", "color.frag");
    Mesh mesh(quad(), quadIndices());
    while (app.begin()) {
        app.clear();
        shader.use();
        glm::mat4 mvp(1.f);
        // 按住空格看线框：一个矩形由两个三角形组成。
        glPolygonMode(GL_FRONT_AND_BACK,
                      glfwGetKey(app.window, GLFW_KEY_SPACE) == GLFW_PRESS ? GL_LINE : GL_FILL);
        shader.set("mvp", mvp);
        shader.set("pulse", 1.f);
        mesh.draw();
        app.end();
    }
    return 0;
} catch (const std::exception& e) {
    return failure(e);
}
