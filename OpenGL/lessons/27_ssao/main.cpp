// 27_ssao
// 本目录拥有完整课程源码；只共享第三方依赖，不链接其他章节的课程实现。
#include "support/advanced.hpp"

// SSAO 与降噪，先阅读本目录 README.md。
// 本课重点：读取 G-buffer 估算邻域遮挡，经降噪后只调制环境光。
int main(int argc, char** argv) try {
    // 先创建上下文；它必须比 shader、纹理和网格活得更久。
    App app(argc, argv, "27_ssao - OpenGL Course");
    RenderTarget gbuffer(3), occlusion, filtered;
    Shader geometry(app, "advanced.vert", "gbuffer.frag"),
        lighting(app, "screen.vert", "deferred.frag"), ao(app, "screen.vert", "ssao.frag"),
        blur(app, "screen.vert", "ao_blur.frag");
    Mesh box(cube()), ball(sphere(24, 12)), plane(quad(), quadIndices());
    Camera camera;
    camera.pos = {0, 3, 8};
    camera.pitch = -18;

    // 固定随机种子生成朝向法线半球的样本，靠近原点的样本更密集。
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
    // 每帧处理输入并绘制；GPU 对象在退出循环后按 C++ 作用域逆序释放。
    while (app.begin()) {
        if (app.width < 1 || app.height < 1) {
            app.end();
            continue;
        }
        camera.update(app);
        gbuffer.resize(app.width, app.height);
        occlusion.resize(app.width, app.height);
        filtered.resize(app.width, app.height);

        // 第一遍：输出位置、法线和材质，后续屏幕空间效果共用这些附件。
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

        // 第二遍：从 G-buffer 和噪声构造采样方向，估算局部遮挡。
        occlusion.bind();
        ao.use();
        ao.set("positionTex", 0);
        ao.set("normalTex", 1);
        ao.set("noiseTex", 2);
        ao.set("projection", camera.projection(app));
        for (int k = 0; k < 32; ++k)
            ao.set(("samples[" + std::to_string(k) + "]").c_str(), kernel[k]);
        bind2D(gbuffer.colors[0], 0);
        bind2D(gbuffer.colors[1], 1);
        bind2D(noiseTexture, 2);
        plane.draw();

        // 第三遍：按几何边界降噪，减少随机样本产生的颗粒感。
        filtered.bind();
        blur.use();
        blur.set("tex", 0);
        blur.set("positionTex", 1);
        blur.set("normalTex", 2);
        bind2D(occlusion.colors[0], 0);
        bind2D(gbuffer.colors[0], 1);
        bind2D(gbuffer.colors[1], 2);
        plane.draw();

        // 最后一遍：合成光照；AO 仅调制环境项，可用按键查看中间结果。
        defaultTarget(app);
        app.clear();
        lighting.use();
        lighting.set("positionTex", 0);
        lighting.set("normalTex", 1);
        lighting.set("albedoTex", 2);
        lighting.set("aoTex", 3);
        lighting.set("view", camera.view());
        lighting.set("useAO", app.key(GLFW_KEY_SPACE) != GLFW_PRESS);
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
