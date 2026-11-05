// 11_framebuffer
// 本目录拥有完整课程源码；只共享第三方依赖，不链接其他章节的课程实现。
#include "support/framebuffer.hpp"

// 离屏渲染与后处理：配合本目录 README.md 阅读。
// 本课重点：先绘制到离屏纹理，再将纹理用于全屏后处理。
int main(int argc, char** argv) try {
    // 先创建上下文；它必须比 shader、纹理和网格活得更久。
    App app(argc, argv, "11_framebuffer - OpenGL Course");
    Shader scene(app, "basic.vert", "texture.frag"), screen(app, "screen.vert", "screen.frag");
    Mesh box(cube()), plane(quad(), quadIndices());
    Texture texture;
    Camera camera;
    Framebuffer target;
    // 每帧处理输入并绘制；GPU 对象在退出循环后按 C++ 作用域逆序释放。
    while (app.begin()) {
        if (app.width == 0 || app.height == 0) {
            app.end();
            continue;
        }
        camera.update(app);
        target.resize(app.width, app.height);
        // 第一遍：向离屏颜色纹理和深度附件画立方体。
        glBindFramebuffer(GL_FRAMEBUFFER, target.fbo);
        glEnable(GL_DEPTH_TEST);
        app.clear();
        scene.use();
        texture.bind();
        scene.set("tex", 0);
        auto model = glm::rotate(glm::mat4(1), app.time(), glm::vec3(.3f, 1, 0));
        scene.set("mvp", camera.projection(app) * camera.view() * model);
        box.draw();
        // 第二遍：默认帧缓冲上的全屏矩形。按住空格显示原图，松开为灰度。
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDisable(GL_DEPTH_TEST);
        app.clear();
        screen.use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, target.color);
        screen.set("tex", 0);
        screen.set("grayscale", glfwGetKey(app.window, GLFW_KEY_SPACE) != GLFW_PRESS);
        plane.draw();
        app.end();
    }
    return 0;
} catch (const std::exception& e) {
    return failure(e);
}
