// 02_triangle
// 本目录拥有完整课程源码；只共享第三方依赖，不链接其他章节的课程实现。
#include "support/app.hpp"

// 第一个彩色三角形：配合本目录 README.md 阅读。
// 本课重点：连接 Shader、VAO 和顶点数据，完成第一次 draw call。
int main(int argc, char** argv) try {
    // 先创建上下文；它必须比 shader、纹理和网格活得更久。
    App app(argc, argv, "02_triangle - OpenGL Course");
    // shader 路径指向本课自己的文件；属性、uniform 与下方 C++ 设置对应。
    Shader shader(app, "basic.vert", "color.frag");
    // CPU 顶点数据上传后，由 VAO 保存属性布局；布局必须匹配顶点 shader。
    Mesh mesh(
        {-.7f, -.5f, 0, 1, 0, 0, 0, 0, .7f, -.5f, 0, 0, 1, 0, 1, 0, 0, .6f, 0, 0, 0, 1, .5f, 1});
    // 每帧处理输入并绘制；GPU 对象在退出循环后按 C++ 作用域逆序释放。
    while (app.begin()) {
        app.clear();
        shader.use();
        glm::mat4 mvp(1.f);
        shader.set("mvp", mvp);
        shader.set("pulse", 1.f);
        mesh.draw();
        app.end();
    }
    return 0;
} catch (const std::exception& e) {
    return failure(e);
}
