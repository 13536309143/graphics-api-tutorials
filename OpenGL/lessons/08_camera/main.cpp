// 08_camera
// 本目录拥有完整课程源码；只共享第三方依赖，不链接其他章节的课程实现。
#include "support/app.hpp"

// 自由相机：配合本目录 README.md 阅读。
// 本课重点：更新相机位置与朝向，再把 view/projection 传入顶点着色器。
int main(int argc, char** argv) try {
    // 先创建上下文；它必须比 shader、纹理和网格活得更久。
    App app(argc, argv, "08_camera - OpenGL Course");
    // shader 路径指向本课自己的文件；属性、uniform 与下方 C++ 设置对应。
    Shader shader(app, "basic.vert", "texture.frag");
    // CPU 顶点数据上传后，由 VAO 保存属性布局；布局必须匹配顶点 shader。
    Mesh mesh(cube());
    Texture texture;
    Camera camera;
    glEnable(GL_DEPTH_TEST);
    // 每帧处理输入并绘制；GPU 对象在退出循环后按 C++ 作用域逆序释放。
    while (app.begin()) {
        app.clear();
        shader.use();
        texture.bind();
        shader.set("tex", 0);
        camera.update(app);
        auto vp = camera.projection(app) * camera.view();
        for (int n = 0; n < 10; ++n) {
            auto model =
                glm::translate(glm::mat4(1), glm::vec3((n % 5 - 2) * 1.6f, 0, -(n / 5) * 2.f));
            model = glm::rotate(model, app.time() * .3f + n, glm::vec3(1, .5f, .2f));
            shader.set("mvp", vp * model);
            mesh.draw();
        }
        app.end();
    }
    return 0;
} catch (const std::exception& e) {
    return failure(e);
}
