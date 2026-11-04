#include "model.hpp"
#include "environment.hpp"

// 综合实战：PBR 模型展厅，先阅读本目录 README.md。
// 本课重点：组合模型、PBR 和 IBL，观察各渲染步骤共享哪些资源与状态。
int main(int argc, char** argv) try {
    App app(argc, argv, "34_gallery - OpenGL Course");
    Environment env(app);
    Model robot(asset(app, "models/robot.gltf"));
    Mesh ball(sphere()), box(cube()), plane(quad(), quadIndices());
    Camera camera;
    camera.pos = {0, 1, 10};
    camera.pitch = -6;
    Shader model(app, "model.vert", "pbr.frag"), pbr(app, "advanced.vert", "pbr.frag"),
        sky(app, "sky.vert", "sky.frag"), bright(app, "screen.vert", "bright.frag"),
        blur(app, "screen.vert", "blur.frag"), tone(app, "screen.vert", "tone.frag");
    RenderTarget scene, highlights, ping, pong;
    while (app.begin()) {
        if (app.width < 1 || app.height < 1) {
            app.end();
            continue;
        }
        camera.update(app);
        scene.resize(app.width, app.height);
        highlights.resize(app.width, app.height);
        ping.resize(app.width, app.height);
        pong.resize(app.width, app.height);
        scene.bind();
        glEnable(GL_DEPTH_TEST);
        app.clear();
        env.bindIBL();
        model.use();
        pbrDefaults(model);
        cameraUniforms(model, app, camera);
        model.set("useIBL", 1);
        model.set("displayEncode", 0);
        robot.draw(model, glm::rotate(glm::mat4(1), -.35f, glm::vec3(0, 1, 0)));
        pbr.use();
        pbrDefaults(pbr);
        cameraUniforms(pbr, app, camera);
        pbr.set("useIBL", 1);
        pbr.set("displayEncode", 0);
        for (int n = 0; n < 6; ++n) {
            auto m = glm::translate(glm::mat4(1),
                                    glm::vec3((n < 3 ? -3.f : 3.f), float(n % 3 - 1) * 1.8f, 0));
            pbr.set("model", glm::scale(m, glm::vec3(.65f)));
            pbr.set("metallic", n < 3 ? 0.f : 1.f);
            pbr.set("roughness", .1f + .35f * (n % 3));
            ball.draw();
        }
        env.sky(sky, box, app, camera, false);
        glDisable(GL_DEPTH_TEST);
        highlights.bind();
        bright.use();
        bright.set("tex", 0);
        bind2D(scene.colors[0], 0);
        plane.draw();
        GLuint blurred = highlights.colors[0];
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
        tone.use();
        tone.set("tex", 0);
        tone.set("bloomTex", 1);
        tone.set("exposure", 1.f);
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
