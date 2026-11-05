// 10_materials
// 本目录拥有完整课程源码；只共享第三方依赖，不链接其他章节的课程实现。
#include "support/app.hpp"

// 材质与多光源：配合本目录 README.md 阅读。
// 本课重点：区分环境、漫反射和镜面参数，观察材质对高光的影响。
int main(int argc, char** argv) try {
    // 先创建上下文；它必须比 shader、纹理和网格活得更久。
    App app(argc, argv, "10_materials - OpenGL Course");
    // shader 路径指向本课自己的文件；属性、uniform 与下方 C++ 设置对应。
    Shader shader(app, "lit.vert", "lit.frag");
    // CPU 顶点数据上传后，由 VAO 保存属性布局；布局必须匹配顶点 shader。
    Mesh mesh(cube());
    Texture texture;
    Camera camera;
    glEnable(GL_DEPTH_TEST);
    // 每帧处理输入并绘制；GPU 对象在退出循环后按 C++ 作用域逆序释放。
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
        shader.set("textured", 1);
        shader.set("multi", 1);
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
