// 16_blending
// 本目录拥有完整课程源码；只共享第三方依赖，不链接其他章节的课程实现。
#include "support/advanced.hpp"

// 透明混合与排序，先阅读本目录 README.md。
// 本课重点：先画不透明物体，再按距离排序透明物体并进行混合。
int main(int argc, char** argv) try {
    // 先创建上下文；它必须比 shader、纹理和网格活得更久。
    App app(argc, argv, "16_blending - OpenGL Course");
    // shader 路径指向本课自己的文件；属性、uniform 与下方 C++ 设置对应。
    Shader shader(app, "advanced.vert", "surface.frag");
    Mesh box(cube()), plane(quad(), quadIndices());
    Camera camera;
    ImageTexture glass(asset(app, "textures/glass.png"), true);
    glEnable(GL_DEPTH_TEST);
    std::vector<glm::vec3> panes = {{-.7f, 0, 1}, {.4f, .1f, 2}, {.9f, -.2f, 0}};
    // 每帧处理输入并绘制；GPU 对象在退出循环后按 C++ 作用域逆序释放。
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
