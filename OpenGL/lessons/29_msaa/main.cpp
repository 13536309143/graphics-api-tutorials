#include "multisample.hpp"

// 离屏 MSAA 与 Resolve，先阅读本目录 README.md。
// 本课重点：绘制到多重采样附件，再 resolve 到单采样目标后显示。
int main(int argc, char** argv) try {
    App app(argc, argv, "29_msaa - OpenGL Course");
    Multisample msaa;
    RenderTarget resolved(1, false);
    Mesh box(cube()), plane(quad(), quadIndices());
    Shader shader(app, "advanced.vert", "solid.frag"), screen(app, "screen.vert", "screen.frag");
    Camera camera;
    while (app.begin()) {
        if (app.width < 1 || app.height < 1) {
            app.end();
            continue;
        }
        camera.update(app);
        resolved.resize(app.width, app.height);
        msaa.resize(app.width, app.height);
        glBindFramebuffer(GL_FRAMEBUFFER, msaa.fbo);
        glEnable(GL_DEPTH_TEST);
        if (app.key(GLFW_KEY_SPACE) == GLFW_PRESS)
            glDisable(GL_MULTISAMPLE);
        else
            glEnable(GL_MULTISAMPLE);
        app.clear();
        shader.use();
        cameraUniforms(shader, app, camera);
        shader.set("model", glm::rotate(glm::mat4(1), .6f, glm::vec3(.2f, 1, .4f)));
        shader.set("baseColor", glm::vec3(.9f, .8f, .2f));
        shader.set("opacity", 1.f);
        box.draw();
        glBindFramebuffer(GL_READ_FRAMEBUFFER, msaa.fbo);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, resolved.fbo);
        glBlitFramebuffer(0,
                          0,
                          app.width,
                          app.height,
                          0,
                          0,
                          app.width,
                          app.height,
                          GL_COLOR_BUFFER_BIT,
                          GL_NEAREST);
        defaultTarget(app);
        glDisable(GL_DEPTH_TEST);
        screen.use();
        screen.set("tex", 0);
        screen.set("grayscale", 0);
        bind2D(resolved.colors[0], 0);
        plane.draw();
        app.end();
    }
    return 0;
} catch (const std::exception& e) {
    return failure(e);
}
