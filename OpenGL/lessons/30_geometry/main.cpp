#include "advanced.hpp"

// 几何着色器，先阅读本目录 README.md。
// 本课重点：在顶点与光栅化之间加入 geometry shader，按三角形处理顶点。
int main(int argc, char** argv) try {
    App app(argc, argv, "30_geometry - OpenGL Course");
    Shader shader(app, "explode.vert", "surface.frag");
    geometryStage(shader, app, "explode.geom");
    Mesh mesh(cube());
    Camera camera;
    glEnable(GL_DEPTH_TEST);
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
