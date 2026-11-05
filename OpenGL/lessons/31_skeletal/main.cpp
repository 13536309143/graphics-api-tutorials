// 31_skeletal
// 本目录拥有完整课程源码；只共享第三方依赖，不链接其他章节的课程实现。
#include "support/model.hpp"

// 骨骼动画与蒙皮，先阅读本目录 README.md。
// 本课重点：更新动画节点层级，再通过骨骼矩阵与权重进行顶点蒙皮。
int main(int argc, char** argv) try {
    // 先创建上下文；它必须比 shader、纹理和网格活得更久。
    App app(argc, argv, "31_skeletal - OpenGL Course");
    auto modelPath = asset(app, "models/bending.gltf");
    for (int k = 1; k + 1 < argc; ++k)
        if (std::string(argv[k]) == "--model")
            modelPath = std::filesystem::absolute(argv[++k]);
    // 模型持有自己的网格/材质；加载完成不代表自动绑定到当前 shader。
    Model model(modelPath);
    // shader 路径指向本课自己的文件；属性、uniform 与下方 C++ 设置对应。
    Shader shader(app, "model.vert", "pbr.frag");
    Camera camera;
    camera.pos = glm::vec3(0, 1.5f, 6);
    camera.pitch = -6;
    camera.yaw = -90;
    glEnable(GL_DEPTH_TEST);
    // 每帧处理输入并绘制；GPU 对象在退出循环后按 C++ 作用域逆序释放。
    while (app.begin()) {
        camera.update(app);
        app.clear();
        shader.use();
        pbrDefaults(shader);
        cameraUniforms(shader, app, camera);
        // 空格按下时暂停在 bind pose，便于对照骨骼形变。
        model.update(app.key(GLFW_KEY_SPACE) == GLFW_PRESS ? 0.f : app.time());
        model.draw(shader);
        app.end();
    }
    return 0;
} catch (const std::exception& e) {
    return failure(e);
}
