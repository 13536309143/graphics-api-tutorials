// 30_geometry
// 本目录拥有完整课程源码；只共享第三方依赖，不链接其他章节的课程实现。
#include "support/advanced.hpp"

// 几何着色器，先阅读本目录 README.md。
// 本课重点：在顶点与光栅化之间加入 geometry shader，按三角形处理顶点。
int main(int argc, char** argv) try {
    // 先创建上下文；它必须比 shader、纹理和网格活得更久。
    App app(argc, argv, "30_geometry - OpenGL Course");
    // shader 路径指向本课自己的文件；属性、uniform 与下方 C++ 设置对应。
    Shader shader(app, "explode.vert", "surface.frag");
    geometryStage(shader, app, "explode.geom");
    // CPU 顶点数据上传后，由 VAO 保存属性布局；布局必须匹配顶点 shader。
    Mesh mesh(cube());
    Camera camera;
    glEnable(GL_DEPTH_TEST);
    // 每帧处理输入并绘制；GPU 对象在退出循环后按 C++ 作用域逆序释放。
    while (app.begin()) {
        camera.update(app);
        app.clear();
        shader.use();
        surfaceDefaults(shader);
        cameraUniforms(shader, app, camera);
        shader.set("time", app.time());
        shader.set("model", glm::rotate(glm::mat4(1), app.time() * .2f, glm::vec3(0, 1, 0)));
        mesh.draw();
        app.end();
    }
    return 0;
} catch (const std::exception& e) {
    return failure(e);
}
