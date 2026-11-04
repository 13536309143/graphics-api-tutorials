#include "environment.hpp"

// 天空盒与环境反射，先阅读本目录 README.md。
// 本课重点：以反射方向采样 cubemap，观察环境反射与天空盒。
int main(int argc, char** argv) try {
    App app(argc, argv, "18_cubemap - OpenGL Course");
    Environment env(app, false);
    Shader shader(app, "advanced.vert", "reflect.frag"), sky(app, "sky.vert", "sky.frag");
    Mesh ball(sphere()), box(cube());
    Camera camera;
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
