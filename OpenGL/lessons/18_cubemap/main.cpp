// 18_cubemap
// 本目录拥有完整课程源码；只共享第三方依赖，不链接其他章节的课程实现。
#include "support/environment.hpp"

// 天空盒与环境反射，先阅读本目录 README.md。
// 本课重点：以反射方向采样 cubemap，观察环境反射与天空盒。
int main(int argc, char** argv) try {
    // 先创建上下文；它必须比 shader、纹理和网格活得更久。
    App app(argc, argv, "18_cubemap - OpenGL Course");
    Environment env(app, false);
    // shader 路径指向本课自己的文件；属性、uniform 与下方 C++ 设置对应。
    Shader shader(app, "advanced.vert", "reflect.frag"), sky(app, "sky.vert", "sky.frag");
    Mesh ball(sphere()), box(cube());
    Camera camera;
    // 每帧处理输入并绘制；GPU 对象在退出循环后按 C++ 作用域逆序释放。
    while (app.begin()) {
        camera.update(app);
        app.clear();
        shader.use();
        cameraUniforms(shader, app, camera);
        shader.set("environmentMap", 0);
        bindCube(env.environment, 0);
        shader.set("model", glm::mat4(1));
        ball.draw();
        env.sky(sky, box, app, camera);
        app.end();
    }
    return 0;
} catch (const std::exception& e) {
    return failure(e);
}
