// 23_pbr
// 本目录拥有完整课程源码；只共享第三方依赖，不链接其他章节的课程实现。
#include "support/advanced.hpp"

// Cook-Torrance PBR，先阅读本目录 README.md。
// 本课重点：按金属度和粗糙度绘制球阵列，观察直接光 BRDF。
int main(int argc, char** argv) try {
    // 先创建上下文；它必须比 shader、纹理和网格活得更久。
    App app(argc, argv, "23_pbr - OpenGL Course");
    // shader 路径指向本课自己的文件；属性、uniform 与下方 C++ 设置对应。
    Shader shader(app, "advanced.vert", "pbr.frag");
    Mesh ball(sphere());
    Camera camera;
    camera.pos = {0, 0, 14};
    camera.pitch = 0;
    ImageTexture albedo(asset(app, "textures/albedo.png"), true),
        normal(asset(app, "textures/normal.png")), mr(asset(app, "textures/metalrough.png")),
        ao(asset(app, "textures/ao.png"));
    glEnable(GL_DEPTH_TEST);
    // 每帧处理输入并绘制；GPU 对象在退出循环后按 C++ 作用域逆序释放。
    while (app.begin()) {
        camera.update(app);
        app.clear();
        shader.use();
        pbrDefaults(shader);
        cameraUniforms(shader, app, camera);
        albedo.bind(0);
        normal.bind(1);
        mr.bind(2);
        ao.bind(3);
        shader.set("useMaps", 0);
        shader.set("useNormal", 0);
        for (int y = 0; y < 5; ++y)
            for (int x = 0; x < 7; ++x) {
                shader.set("metallic", float(y) / 4);
                shader.set("roughness", std::max(.05f, float(x) / 6));
                auto model =
                    glm::translate(glm::mat4(1), glm::vec3((x - 3) * 1.65f, (y - 2) * 1.65f, 0));
                shader.set("model", glm::scale(model, glm::vec3(.65f)));
                ball.draw();
            }
        app.end();
    }
    return 0;
} catch (const std::exception& e) {
    return failure(e);
}
