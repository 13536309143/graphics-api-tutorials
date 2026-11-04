#include "model.hpp"

// glTF 场景与 PBR 材质，先阅读本目录 README.md。
// 本课重点：导入 glTF 场景并处理节点变换、材质和嵌入贴图。
int main(int argc, char** argv) try {
    App app(argc, argv, "15_model_gltf - OpenGL Course");
    auto modelPath = asset(app, "models/robot.gltf");
    for (int k = 1; k + 1 < argc; ++k)
        if (std::string(argv[k]) == "--model")
            modelPath = std::filesystem::absolute(argv[++k]);
    Model model(modelPath);
    Shader shader(app, "model.vert", "pbr.frag");
    Camera camera;
    camera.pos = glm::vec3(3, 1.0f, 6);
    camera.pitch = -7;
    camera.yaw = -116.565f;
    glEnable(GL_DEPTH_TEST);
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
