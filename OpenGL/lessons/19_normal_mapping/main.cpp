// 19_normal_mapping
// 本目录拥有完整课程源码；只共享第三方依赖，不链接其他章节的课程实现。
#include "support/advanced.hpp"

// 法线贴图与 TBN，先阅读本目录 README.md。
// 本课重点：把法线贴图的切线空间法线转换到光照计算空间。
int main(int argc, char** argv) try {
    // 先创建上下文；它必须比 shader、纹理和网格活得更久。
    App app(argc, argv, "19_normal_mapping - OpenGL Course");
    // shader 路径指向本课自己的文件；属性、uniform 与下方 C++ 设置对应。
    Shader shader(app, "advanced.vert", "surface.frag");
    Mesh plane(quad(), quadIndices());
    Camera camera;
    camera.pos = {0, 0, 3};
    camera.pitch = 0;
    ImageTexture albedo(asset(app, "textures/albedo.png"), true),
        normal(asset(app, "textures/normal.png")), height(asset(app, "textures/height.png"));
    glEnable(GL_DEPTH_TEST);
    // 每帧处理输入并绘制；GPU 对象在退出循环后按 C++ 作用域逆序释放。
    while (app.begin()) {
        camera.update(app);
        app.clear();
        shader.use();
        surfaceDefaults(shader);
        cameraUniforms(shader, app, camera);
        albedo.bind(0);
        normal.bind(1);
        height.bind(2);
        shader.set("useTexture", 1);
        shader.set("baseColor", glm::vec3(1));
        shader.set("useNormal", app.key(GLFW_KEY_SPACE) != GLFW_PRESS);
        shader.set("useParallax", 0);
        shader.set("model", glm::scale(glm::mat4(1), glm::vec3(2)));
        plane.draw();
        app.end();
    }
    return 0;
} catch (const std::exception& e) {
    return failure(e);
}
