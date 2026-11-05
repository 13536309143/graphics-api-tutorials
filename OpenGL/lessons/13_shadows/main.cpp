// 13_shadows
// 本目录拥有完整课程源码；只共享第三方依赖，不链接其他章节的课程实现。
#include "support/shadow.hpp"

// 阴影贴图：配合本目录 README.md 阅读。
// 本课重点：先从光源视角保存深度，再从相机视角比较阴影深度。
int main(int argc, char** argv) try {
    // 先创建上下文；它必须比 shader、纹理和网格活得更久。
    App app(argc, argv, "13_shadows - OpenGL Course");
    Shader depth(app, "depth.vert", "depth.frag"), lit(app, "lit.vert", "lit.frag");
    // CPU 顶点数据上传后，由 VAO 保存属性布局；布局必须匹配顶点 shader。
    Mesh mesh(cube());
    Texture texture;
    Camera camera;
    ShadowMap shadow;
    camera.pos = glm::vec3(0, 3.5f, 7);
    camera.pitch = -23;
    glEnable(GL_DEPTH_TEST);
    auto drawScene = [&](const Shader& s) {
        auto floor = glm::translate(glm::mat4(1), glm::vec3(0, -1, 0));
        floor = glm::scale(floor, glm::vec3(10, .2f, 10));
        s.set("model", floor);
        mesh.draw();
        auto box = glm::rotate(glm::mat4(1), app.time() * .3f, glm::vec3(0, 1, 0));
        s.set("model", box);
        mesh.draw();
        s.set("model", glm::translate(glm::mat4(1), glm::vec3(2, 0, -2)));
        mesh.draw();
    };
    // 每帧处理输入并绘制；GPU 对象在退出循环后按 C++ 作用域逆序释放。
    while (app.begin()) {
        camera.update(app);
        glm::vec3 light(-3, 6, 3);
        auto lightSpace = glm::ortho(-7.f, 7.f, -7.f, 7.f, 1.f, 20.f) *
                          glm::lookAt(light, glm::vec3(0), glm::vec3(0, 1, 0));
        // 同一场景从光源视角渲染深度，再从相机视角比较遮挡。
        glViewport(0, 0, ShadowMap::size, ShadowMap::size);
        glBindFramebuffer(GL_FRAMEBUFFER, shadow.fbo);
        glClear(GL_DEPTH_BUFFER_BIT);
        depth.use();
        depth.set("lightSpace", lightSpace);
        drawScene(depth);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, app.width, app.height);
        app.clear();
        lit.use();
        texture.bind(0);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, shadow.depth);
        lit.set("tex", 0);
        lit.set("shadowMap", 1);
        lit.set("textured", 1);
        lit.set("multi", 0);
        lit.set("shadows", glfwGetKey(app.window, GLFW_KEY_SPACE) != GLFW_PRESS);
        lit.set("view", camera.view());
        lit.set("projection", camera.projection(app));
        lit.set("lightSpace", lightSpace);
        lit.set("eye", camera.pos);
        lit.set("light", light);
        lit.set("baseColor", glm::vec3(.9f));
        lit.set("shininess", 32.f);
        drawScene(lit);
        app.end();
    }
    return 0;
} catch (const std::exception& e) {
    return failure(e);
}
