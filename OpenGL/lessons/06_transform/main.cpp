// 06_transform
// 本目录拥有完整课程源码；只共享第三方依赖，不链接其他章节的课程实现。
#include "support/app.hpp"

// 二维变换：配合本目录 README.md 阅读。
// 本课重点：按模型、视图、投影顺序变换顶点，留意矩阵乘法顺序。
int main(int argc, char** argv) try {
    // 先创建上下文；它必须比 shader、纹理和网格活得更久。
    App app(argc, argv, "06_transform - OpenGL Course");
    // shader 路径指向本课自己的文件；属性、uniform 与下方 C++ 设置对应。
    Shader shader(app, "basic.vert", "texture.frag");
    // CPU 顶点数据上传后，由 VAO 保存属性布局；布局必须匹配顶点 shader。
    Mesh mesh(quad(), quadIndices());
    Texture texture;
    // 每帧处理输入并绘制；GPU 对象在退出循环后按 C++ 作用域逆序释放。
    while (app.begin()) {
        app.clear();
        shader.use();
        glm::mat4 mvp(1.f);
        mvp = glm::translate(mvp, glm::vec3(.25f, 0, 0));
        mvp = glm::rotate(mvp, app.time(), glm::vec3(0, 0, 1));
        mvp = glm::scale(mvp, glm::vec3(.6f));
        shader.set("mvp", mvp);
        texture.bind();
        shader.set("tex", 0);
        mesh.draw();
        app.end();
    }
    return 0;
} catch (const std::exception& e) {
    return failure(e);
}
