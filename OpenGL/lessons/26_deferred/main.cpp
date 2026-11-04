#include "advanced.hpp"

// 延迟渲染与 G-buffer，先阅读本目录 README.md。
// 本课重点：几何遍写入 G-buffer，光照遍按屏幕像素读取材质和位置。
int main(int argc, char** argv) try {
    App app(argc, argv, "26_deferred - OpenGL Course");
    RenderTarget gbuffer(3), occlusion, filtered;
    Shader geometry(app, "advanced.vert", "gbuffer.frag"),
        lighting(app, "screen.vert", "deferred.frag"), ao(app, "screen.vert", "ssao.frag"),
        blur(app, "screen.vert", "ao_blur.frag");
    Mesh box(cube()), ball(sphere(24, 12)), plane(quad(), quadIndices());
    Camera camera;
    camera.pos = {0, 3, 8};
    camera.pitch = -18;
    std::mt19937 random(42);
    std::uniform_real_distribution<float> unit(0, 1);
    std::vector<glm::vec3> kernel, noise;
    for (int k = 0; k < 32; ++k) {
        auto p =
            glm::normalize(glm::vec3(unit(random) * 2 - 1, unit(random) * 2 - 1, unit(random)));
        p *= unit(random);
        float f = float(k) / 31;
        p *= .1f + .9f * f * f;
        kernel.push_back(p);
    }
    for (int k = 0; k < 16; ++k)
        noise.emplace_back(unit(random) * 2 - 1, unit(random) * 2 - 1, 0);
    GLuint noiseTexture;
    glGenTextures(1, &noiseTexture);
    bind2D(noiseTexture, 2);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, 4, 4, 0, GL_RGB, GL_FLOAT, noise.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    while (app.begin()) {
        if (app.width < 1 || app.height < 1) {
            app.end();
            continue;
        }
        camera.update(app);
        gbuffer.resize(app.width, app.height);
        occlusion.resize(app.width, app.height);
        filtered.resize(app.width, app.height);

        // 几何遍先写 G-buffer，随后全屏光照遍重用屏幕空间数据。
        gbuffer.bind();
        glEnable(GL_DEPTH_TEST);
        glClearColor(0, 0, 0, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        geometry.use();
        cameraUniforms(geometry, app, camera);
        geometry.set(
            "model",
            glm::scale(glm::translate(glm::mat4(1), glm::vec3(0, -1, 0)), glm::vec3(12, .2f, 12)));
        geometry.set("baseColor", glm::vec3(.5f));
        box.draw();
        for (int z = 0; z < 3; ++z)
            for (int x = 0; x < 5; ++x) {
                auto model =
                    glm::translate(glm::mat4(1), glm::vec3((x - 2) * 1.6f, -.25f, -z * 1.7f));
                geometry.set("model", glm::scale(model, glm::vec3(.65f)));
                geometry.set("baseColor", glm::vec3(.15f + .12f * x, .3f, .7f - .12f * z));
                ball.draw();
            }
        glDisable(GL_DEPTH_TEST);
        defaultTarget(app);
        app.clear();
        lighting.use();
        lighting.set("positionTex", 0);
        lighting.set("normalTex", 1);
        lighting.set("albedoTex", 2);
        lighting.set("aoTex", 3);
        lighting.set("view", camera.view());
        lighting.set("useAO", 0);
        lighting.set("debugView",
                     glfwGetKey(app.window, GLFW_KEY_1) == GLFW_PRESS   ? 1
                     : glfwGetKey(app.window, GLFW_KEY_2) == GLFW_PRESS ? 2
                                                                        : 0);
        bind2D(gbuffer.colors[0], 0);
        bind2D(gbuffer.colors[1], 1);
        bind2D(gbuffer.colors[2], 2);
        bind2D(filtered.colors[0], 3);
        plane.draw();
        app.end();
    }
    glDeleteTextures(1, &noiseTexture);
    return 0;
} catch (const std::exception& e) {
    return failure(e);
}
