#include "advanced.hpp"

// Bloom 与高斯模糊，先阅读本目录 README.md。
// 本课重点：从 HDR 提取亮部，交替模糊后再与原始场景合成。
int main(int argc, char** argv) try {
    App app(argc, argv, "22_bloom - OpenGL Course");
    RenderTarget scene(2), ping, pong;
    Shader shader(app, "advanced.vert", "hdr_scene.frag"), tone(app, "screen.vert", "tone.frag"),
        blur(app, "screen.vert", "blur.frag");
    Mesh ball(sphere()), plane(quad(), quadIndices());
    Camera camera;
    camera.pos = {0, 0, 8};
    camera.pitch = 0;
    float exposure = 1;

    // 每帧依次完成 HDR 场景、高亮提取、交替模糊与最终合成。
    while (app.begin()) {
        if (app.width < 1 || app.height < 1) {
            app.end();
            continue;
        }
        camera.update(app);
        if (glfwGetKey(app.window, GLFW_KEY_UP) == GLFW_PRESS)
            exposure += app.dt;
        if (glfwGetKey(app.window, GLFW_KEY_DOWN) == GLFW_PRESS)
            exposure = std::max(.05f, exposure - app.dt);
        scene.resize(app.width, app.height);
        ping.resize(app.width, app.height);
        pong.resize(app.width, app.height);
        scene.bind();
        glEnable(GL_DEPTH_TEST);
        app.clear();
        shader.use();
        cameraUniforms(shader, app, camera);
        for (int n = 0; n < 3; ++n) {
            auto model = glm::translate(glm::mat4(1), glm::vec3((n - 1) * 2.1f, 0, 0));
            shader.set("model", glm::scale(model, glm::vec3(.7f)));
            shader.set("baseColor",
                       n == 0   ? glm::vec3(.2f, .6f, 1)
                       : n == 1 ? glm::vec3(1, .35f, .08f)
                                : glm::vec3(.2f, 1, .3f));
            shader.set("emission", n == 1 ? 5.f : 0.f);
            ball.draw();
        }
        glDisable(GL_DEPTH_TEST);
        GLuint blurred = scene.colors[1];
        for (int pass = 0; pass < 8; ++pass) {
            auto& target = pass % 2 == 0 ? ping : pong;
            target.bind();
            blur.use();
            blur.set("tex", 0);
            blur.set("horizontal", pass % 2 == 0);
            bind2D(blurred, 0);
            plane.draw();
            blurred = target.colors[0];
        }
        defaultTarget(app);
        app.clear();
        tone.use();
        tone.set("tex", 0);
        tone.set("bloomTex", 1);
        tone.set("exposure", exposure);
        tone.set("toneMap", 1);
        tone.set("bloom", app.key(GLFW_KEY_SPACE) != GLFW_PRESS);
        bind2D(scene.colors[0], 0);
        bind2D(blurred, 1);
        plane.draw();
        app.end();
    }
    return 0;
} catch (const std::exception& e) {
    return failure(e);
}
