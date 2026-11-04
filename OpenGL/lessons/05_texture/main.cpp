#include "app.hpp"

// 纹理与采样：配合本目录 README.md 阅读。
// 本课重点：绑定纹理单元，并让 sampler uniform 指向相同单元。
int main(int argc, char** argv) try {
    App app(argc, argv, "05_texture - OpenGL Course");
    Shader shader(app, "basic.vert", "texture.frag");
    Mesh mesh(quad(), quadIndices());
    Texture texture;
    while (app.begin()) {
        app.clear();
        shader.use();
        glm::mat4 mvp(1.f);
        shader.set("mvp", mvp);
        texture.bind();
        shader.set("tex", 0);
        mesh.draw();
        app.end();
    }
    return 0;
} catch (const std::exception& e) {
    return failure(e);
}
