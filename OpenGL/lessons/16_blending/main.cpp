#include "advanced.hpp"

// 透明混合与排序，先阅读本目录 README.md。
// 本课重点：先画不透明物体，再按距离排序透明物体并进行混合。
int main(int argc, char** argv) try {
    App app(argc, argv, "16_blending - OpenGL Course");
    Shader shader(app, "advanced.vert", "surface.frag");
    Mesh box(cube()), plane(quad(), quadIndices());
    Camera camera;
    ImageTexture glass(asset(app, "textures/glass.png"), true);
    glEnable(GL_DEPTH_TEST);
    std::vector<glm::vec3> panes = {{-.7f, 0, 1}, {.4f, .1f, 2}, {.9f, -.2f, 0}};
    while (app.begin()) {
        camera.update(app);
        app.clear();
        shader.use();
        surfaceDefaults(shader);
        cameraUniforms(shader, app, camera);
        shader.set("model", glm::mat4(1));
        box.draw();
        auto ordered = panes;
        std::sort(ordered.begin(), ordered.end(), [&](auto a, auto b) {
            return glm::dot(camera.pos - a, camera.pos - a) >
                   glm::dot(camera.pos - b, camera.pos - b);
        });
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);
        glass.bind(0);
        shader.set("useTexture", 1);
        shader.set("baseColor", glm::vec3(1));
        for (auto pos : ordered) {
            shader.set("model", glm::translate(glm::mat4(1), pos));
            plane.draw();
        }
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
        app.end();
    }
    return 0;
} catch (const std::exception& e) {
    return failure(e);
}
