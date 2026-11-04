#include "advanced.hpp"

// 模板测试与描边，先阅读本目录 README.md。
// 本课重点：第一遍写 stencil，第二遍用放大模型绘制未被覆盖的描边。
int main(int argc, char** argv) try {
    App app(argc, argv, "17_stencil - OpenGL Course");
    Shader solid(app, "advanced.vert", "surface.frag"), outline(app, "advanced.vert", "solid.frag");
    Mesh mesh(cube());
    Camera camera;
    RenderTarget target(1, false);
    Shader screen(app, "screen.vert", "screen.frag");
    Mesh plane(quad(), quadIndices());
    while (app.begin()) {
        if (app.width < 1 || app.height < 1) {
            app.end();
            continue;
        }
        camera.update(app);
        target.resize(app.width, app.height);
        target.bind();
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_STENCIL_TEST);
        glStencilMask(0xFF);
        app.clear();
        glClear(GL_STENCIL_BUFFER_BIT);
        auto model = glm::rotate(glm::mat4(1), app.time() * .3f, glm::vec3(0, 1, 0));
        glStencilFunc(GL_ALWAYS, 1, 0xFF);
        glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
        solid.use();
        surfaceDefaults(solid);
        cameraUniforms(solid, app, camera);
        solid.set("model", model);
        mesh.draw();
        glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
        glStencilMask(0);
        glDisable(GL_DEPTH_TEST);
        outline.use();
        cameraUniforms(outline, app, camera);
        outline.set("model", glm::scale(model, glm::vec3(1.08f)));
        outline.set("baseColor", glm::vec3(.1f, .8f, 1));
        outline.set("opacity", 1.f);
        mesh.draw();
        glStencilMask(0xFF);
        glDisable(GL_STENCIL_TEST);
        defaultTarget(app);
        app.clear();
        screen.use();
        screen.set("tex", 0);
        screen.set("grayscale", 0);
        bind2D(target.colors[0], 0);
        plane.draw();
        app.end();
    }
    return 0;
} catch (const std::exception& e) {
    return failure(e);
}
