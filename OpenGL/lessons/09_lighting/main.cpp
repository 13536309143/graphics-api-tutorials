#include "app.hpp"

// Phong 光照：配合本目录 README.md 阅读。
// 本课重点：统一法线、光源和视线所在坐标空间，再计算基础光照。
int main(int argc, char** argv) try {
    App app(argc, argv, "09_lighting - OpenGL Course");
    Shader shader(app, "lit.vert", "lit.frag");
    Mesh mesh(cube());
    Texture texture;
    Camera camera;
    glEnable(GL_DEPTH_TEST);
    while (app.begin()) {
        camera.update(app);
        app.clear();
        shader.use();
        texture.bind();
        shader.set("tex", 0);
        shader.set("shadowMap", 1);
        shader.set("view", camera.view());
        shader.set("projection", camera.projection(app));
        shader.set("lightSpace", glm::mat4(1));
        shader.set("shadows", 0);
        shader.set("eye", camera.pos);
        shader.set("light", glm::vec3(2 * std::sin(app.time()), 3, 2 * std::cos(app.time())));
        shader.set("textured", 0);
        shader.set("multi", 0);
        for (int n = 0; n < 3; ++n) {
            auto model = glm::translate(glm::mat4(1), glm::vec3((n - 1) * 1.7f, 0, 0));
            model = glm::rotate(model, app.time() * .2f, glm::vec3(.2f, 1, 0));
            shader.set("model", model);
            shader.set("baseColor", glm::vec3(.9f, .5f + .2f * n, .25f));
            shader.set("shininess", float(8 * (1 << (n * 2))));
            mesh.draw();
        }
        app.end();
    }
    return 0;
} catch (const std::exception& e) {
    return failure(e);
}
