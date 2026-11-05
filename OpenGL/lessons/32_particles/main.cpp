// 32_particles
// 本目录拥有完整课程源码；只共享第三方依赖，不链接其他章节的课程实现。
#include "support/advanced.hpp"

// 实例化粒子，先阅读本目录 README.md。
// 本课重点：实例化绘制粒子，注意 billboard、混合和深度写入之间的关系。
int main(int argc, char** argv) try {
    // 先创建上下文；它必须比 shader、纹理和网格活得更久。
    App app(argc, argv, "32_particles - OpenGL Course");
    // shader 路径指向本课自己的文件；属性、uniform 与下方 C++ 设置对应。
    Shader shader(app, "particle.vert", "particle.frag");
    Mesh plane(quad(), quadIndices());
    Camera camera;
    camera.pos = {0, 2, 7};
    camera.pitch = -8;
    std::mt19937 random(7);
    std::uniform_real_distribution<float> unit(0, 1);
    std::vector<glm::vec3> seeds;
    for (int n = 0; n < 600; ++n)
        seeds.emplace_back(unit(random), unit(random), unit(random));
    GLuint instance;
    glGenBuffers(1, &instance);
    glBindVertexArray(plane.vao);
    glBindBuffer(GL_ARRAY_BUFFER, instance);
    glBufferData(GL_ARRAY_BUFFER, 600 * sizeof(glm::vec4), nullptr, GL_STREAM_DRAW);
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(glm::vec4), nullptr);
    glVertexAttribDivisor(3, 1);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glDepthMask(GL_FALSE);
    // 每帧处理输入并绘制；GPU 对象在退出循环后按 C++ 作用域逆序释放。
    while (app.begin()) {
        camera.update(app);
        glDepthMask(GL_TRUE);
        app.clear();
        glDepthMask(GL_FALSE);
        std::vector<glm::vec4> particles;
        particles.reserve(600);
        for (int n = 0; n < 600; ++n) {
            float age = std::fmod(app.time() * .4f + seeds[n].x, 1.f);
            float angle = seeds[n].y * 6.2831853f + app.time() * .2f;
            float radius = std::sqrt(seeds[n].z) * (.1f + age * .9f);
            particles.emplace_back(
                std::cos(angle) * radius, age * 3, std::sin(angle) * radius, age);
        }
        glBindBuffer(GL_ARRAY_BUFFER, instance);
        glBufferSubData(
            GL_ARRAY_BUFFER, 0, GLsizeiptr(particles.size() * sizeof(glm::vec4)), particles.data());
        shader.use();
        cameraUniforms(shader, app, camera);
        auto right = glm::normalize(glm::cross(camera.front, glm::vec3(0, 1, 0)));
        shader.set("right", right);
        shader.set("up", glm::cross(right, camera.front));
        glBindVertexArray(plane.vao);
        // 绘制使用当前程序、VAO 和附件；属性布局必须匹配顶点着色器。
        glDrawElementsInstanced(GL_TRIANGLES, plane.count, GL_UNSIGNED_INT, nullptr, 600);
        app.end();
    }
    glDepthMask(GL_TRUE);
    glDeleteBuffers(1, &instance);
    return 0;
} catch (const std::exception& e) {
    return failure(e);
}
