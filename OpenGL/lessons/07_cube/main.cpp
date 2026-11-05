// 07_cube
// 本目录拥有完整课程源码；只共享第三方依赖，不链接其他章节的课程实现。
#include "support/app.hpp"

// 三维坐标与深度测试：配合本目录 README.md 阅读。
// 本课重点：开启深度测试并使用透视投影，正确显示前后遮挡。
int main(int argc, char** argv) try {
    // 先创建上下文；它必须比 shader、纹理和网格活得更久。
    App app(argc, argv, "07_cube - OpenGL Course");
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
        auto model = glm::rotate(glm::mat4(1), app.time() * .7f, glm::vec3(.5f, 1, .2f));
        auto view = glm::lookAt(glm::vec3(2, 1.5f, 3), glm::vec3(0), glm::vec3(0, 1, 0));
        auto proj = glm::perspective(glm::radians(45.f), app.aspect(), .1f, 100.f);
        shader.set("mvp", proj * view * model);
        mesh.draw();
        app.end();
    }
    return 0;
} catch (const std::exception& e) {
    return failure(e);
}
