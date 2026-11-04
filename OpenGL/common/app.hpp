#pragma once
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

// 上下文必须比所有 GPU 对象活得更久：在 main 中先创建 App，再创建 Shader/Mesh。
struct App {
    GLFWwindow* window = nullptr;
    int width = 960, height = 640, frames = 0;
    bool variant = false;
    bool smoke = false, expectGeometry = true, capture = false;
    std::filesystem::path assets;
    float dt = 0, last = 0;
    App(int argc, char** argv, const char* title) {
        for (int i = 1; i < argc; ++i) {
            smoke |= std::string(argv[i]) == "--smoke";
            capture |= std::string(argv[i]) == "--capture";
            variant |= std::string(argv[i]) == "--variant";
        }
        expectGeometry = std::string(title).find("01_window") != 0;
        assets = std::filesystem::absolute(argv[0]).parent_path() / "shaders";
        glfwSetErrorCallback([](int c, const char* s) {
            std::cerr << "GLFW " << c << ": " << s << '\n';
        });
        if (!glfwInit())
            throw std::runtime_error("GLFW initialization failed");
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
        if (smoke)
            glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
        window = glfwCreateWindow(width, height, title, nullptr, nullptr);
        if (!window) {
            glfwTerminate();
            throw std::runtime_error("Cannot create OpenGL 3.3 core context");
        }

        // 必须先让上下文在当前线程生效，再由 GLAD 加载 OpenGL 函数入口。
        glfwMakeContextCurrent(window);
        if (!gladLoadGL(glfwGetProcAddress)) {
            glfwDestroyWindow(window);
            glfwTerminate();
            throw std::runtime_error("GLAD load failed");
        }
        glfwSwapInterval(smoke ? 0 : 1);
        std::cout << "GPU: " << glGetString(GL_RENDERER) << "\nOpenGL: " << glGetString(GL_VERSION)
                  << '\n';
        last = float(glfwGetTime());
    }
    ~App() {
        glfwDestroyWindow(window);
        glfwTerminate();
    }
    App(const App&) = delete;
    App& operator=(const App&) = delete;
    int key(int k) const {
        return smoke && variant && k == GLFW_KEY_SPACE ? GLFW_PRESS : glfwGetKey(window, k);
    }
    float time() const {
        return smoke ? 1.0f : float(glfwGetTime());
    }

    // 每帧先处理事件并读取 framebuffer 尺寸；高 DPI 下它可能不同于窗口逻辑尺寸。
    bool begin() {
        if (glfwWindowShouldClose(window) || (smoke && frames >= 4))
            return false;
        glfwPollEvents();
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);
        float now = float(glfwGetTime());
        dt = std::min(now - last, 0.05f);
        last = now;
        glfwGetFramebufferSize(window, &width, &height);
        glViewport(0, 0, width, height);
        return true;
    }
    void end() {

        // 自动验证使用固定帧数并在交换缓冲前读回当前后缓冲，检查是否真正绘出几何。
        if (smoke && frames == 3) {
            std::vector<unsigned char> rgb(size_t(width) * height * 3);
            glPixelStorei(GL_PACK_ALIGNMENT, 1);
            glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, rgb.data());
            glPixelStorei(GL_PACK_ALIGNMENT, 4);
            size_t changed = 0;
            for (size_t i = 0; i < rgb.size(); i += 3)
                if (std::abs(int(rgb[i]) - 9) > 3 || std::abs(int(rgb[i + 1]) - 14) > 3 ||
                    std::abs(int(rgb[i + 2]) - 23) > 3)
                    ++changed;
            std::cout << "Rendered non-background pixels: " << changed << '\n';
            int lo = 255, hi = 0;
            for (size_t k = 0; k < rgb.size(); k += 3) {
                int l = int(rgb[k]) + rgb[k + 1] + rgb[k + 2];
                lo = std::min(lo, l / 3);
                hi = std::max(hi, l / 3);
            }
            if (expectGeometry && (changed < 100 || hi - lo < 10))
                throw std::runtime_error("Smoke test: geometry is missing");
            if (capture) {
                std::ofstream out(assets.parent_path() / (std::string(glfwGetWindowTitle(window)) +
                                                          (variant ? " - variant" : "") + ".ppm"),
                                  std::ios::binary);
                out << "P6\n" << width << ' ' << height << "\n255\n";
                for (int y = height - 1; y >= 0; --y)
                    out.write(reinterpret_cast<const char*>(rgb.data() + size_t(y) * width * 3),
                              width * 3);
            }
        }
        GLenum e = glGetError();
        if (e != GL_NO_ERROR)
            throw std::runtime_error("OpenGL error: " + std::to_string(e));
        glfwSwapBuffers(window);
        ++frames;
    }
    void clear() {
        glClearColor(0.035f, 0.055f, 0.09f, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }
    float aspect() const {
        return float(std::max(width, 1)) / float(std::max(height, 1));
    }
};

// Shader 管理程序对象生命周期；编译失败和链接失败分别保留驱动诊断。
struct Shader {
    GLuint id = 0;
    static std::string read(const std::filesystem::path& p) {
        std::ifstream f(p);
        if (!f)
            throw std::runtime_error("Missing shader: " + p.string());
        std::ostringstream s;
        s << f.rdbuf();
        return s.str();
    }
    static GLuint compile(GLenum type, const std::string& source) {
        GLuint s = glCreateShader(type);
        const char* p = source.c_str();
        glShaderSource(s, 1, &p, nullptr);
        glCompileShader(s);
        GLint ok;
        glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
        if (!ok) {
            char log[4096];
            glGetShaderInfoLog(s, sizeof(log), nullptr, log);
            glDeleteShader(s);
            throw std::runtime_error(log);
        }
        return s;
    }
    Shader(const App& a, const char* vert, const char* frag) {
        const auto vs = read(a.assets / vert), fs = read(a.assets / frag);
        GLuint v = compile(GL_VERTEX_SHADER, vs), f = 0;
        try {
            f = compile(GL_FRAGMENT_SHADER, fs);
        } catch (...) {
            glDeleteShader(v);
            throw;
        }
        id = glCreateProgram();
        glAttachShader(id, v);
        glAttachShader(id, f);
        glLinkProgram(id);

        // 链接后程序保留可执行代码，独立的 shader 对象不再需要。
        glDeleteShader(v);
        glDeleteShader(f);
        GLint ok;
        glGetProgramiv(id, GL_LINK_STATUS, &ok);
        if (!ok) {
            char log[4096];
            glGetProgramInfoLog(id, sizeof(log), nullptr, log);
            glDeleteProgram(id);
            id = 0;
            throw std::runtime_error(log);
        }
    }
    ~Shader() {
        glDeleteProgram(id);
    }
    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;
    void use() const {
        glUseProgram(id);
    }
    GLint loc(const char* n) const {
        return glGetUniformLocation(id, n);
    }
    void set(const char* n, int v) const {
        glUniform1i(loc(n), v);
    }
    void set(const char* n, float v) const {
        glUniform1f(loc(n), v);
    }
    void set(const char* n, const glm::vec3& v) const {
        glUniform3fv(loc(n), 1, glm::value_ptr(v));
    }
    void set(const char* n, const glm::mat4& m) const {
        glUniformMatrix4fv(loc(n), 1, GL_FALSE, glm::value_ptr(m));
    }
};

// 每个顶点 8 个 float：位置 xyz + 颜色/法线 xyz + UV。第 09 课开始把中间三个量作为法线。
struct Mesh {
    GLuint vao = 0, vbo = 0, ebo = 0;
    GLsizei count = 0;
    bool indexed = false;
    Mesh(const std::vector<float>& data, const std::vector<unsigned>& indices = {}) {
        indexed = !indices.empty();
        count = GLsizei(indexed ? indices.size() : data.size() / 8);
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(
            GL_ARRAY_BUFFER, GLsizeiptr(data.size() * sizeof(float)), data.data(), GL_STATIC_DRAW);

        // attribute 描述写入当前 VAO；stride 和 offset 必须与 CPU 顶点布局一致。
        for (GLuint i = 0; i < 3; ++i) {
            glEnableVertexAttribArray(i);
            glVertexAttribPointer(
                i,
                i == 2 ? 2 : 3,
                GL_FLOAT,
                GL_FALSE,
                8 * sizeof(float),
                reinterpret_cast<void*>(size_t(i == 2 ? 6 : i * 3) * sizeof(float)));
        }

        // 索引缓冲绑定也是 VAO 状态，因此要在解绑 VAO 前完成设置。
        if (indexed) {
            glGenBuffers(1, &ebo);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                         GLsizeiptr(indices.size() * sizeof(unsigned)),
                         indices.data(),
                         GL_STATIC_DRAW);
        }
        glBindVertexArray(0);
    }
    ~Mesh() {
        glDeleteBuffers(1, &ebo);
        glDeleteBuffers(1, &vbo);
        glDeleteVertexArrays(1, &vao);
    }
    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;
    void draw() const {
        glBindVertexArray(vao);
        if (indexed)
            glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, nullptr);
        else
            glDrawArrays(GL_TRIANGLES, 0, count);
    }
};
inline std::vector<float> quad() {
    return {-.65f, -.65f, 0, 1,   0.3f, 0.2f, 0, 0, .65f,  -.65f, 0, .2f, 1,   .4f, 1, 0,
            .65f,  .65f,  0, .2f, .5f,  1,    1, 1, -.65f, .65f,  0, 1,   .8f, .2f, 0, 1};
}
inline std::vector<unsigned> quadIndices() {
    return {0, 1, 2, 2, 3, 0};
}
inline std::vector<float> cube() {
    // 每面各有 4 个顶点，因为相邻面的法线/UV 不同；不能只用 8 个顶点。
    const glm::vec3 normals[] = {
        {0, 0, 1}, {0, 0, -1}, {1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}};
    const glm::vec3 points[][4] = {{{-1, -1, 1}, {1, -1, 1}, {1, 1, 1}, {-1, 1, 1}},
                                   {{1, -1, -1}, {-1, -1, -1}, {-1, 1, -1}, {1, 1, -1}},
                                   {{1, -1, 1}, {1, -1, -1}, {1, 1, -1}, {1, 1, 1}},
                                   {{-1, -1, -1}, {-1, -1, 1}, {-1, 1, 1}, {-1, 1, -1}},
                                   {{-1, 1, 1}, {1, 1, 1}, {1, 1, -1}, {-1, 1, -1}},
                                   {{-1, -1, -1}, {1, -1, -1}, {1, -1, 1}, {-1, -1, 1}}};
    const glm::vec2 uv[] = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};
    std::vector<float> v;
    for (int f = 0; f < 6; ++f)
        for (int k : {0, 1, 2, 2, 3, 0}) {
            auto p = points[f][k] * .5f;
            auto n = normals[f];
            auto t = uv[k];
            v.insert(v.end(), {p.x, p.y, p.z, n.x, n.y, n.z, t.x, t.y});
        }
    return v;
}
struct Texture {
    GLuint id = 0;
    Texture() {
        // 用 CPU 生成棋盘纹理，初学时不需要额外的图片解析库和绝对路径。
        std::vector<unsigned char> pixels(64 * 64 * 3);
        for (int y = 0; y < 64; ++y)
            for (int x = 0; x < 64; ++x) {
                bool c = ((x / 8 + y / 8) % 2) == 0;
                size_t i = size_t(y * 64 + x) * 3;
                pixels[i] = c ? 245 : 25;
                pixels[i + 1] = c ? 170 : 110;
                pixels[i + 2] = c ? 55 : 160;
            }
        glGenTextures(1, &id);
        glBindTexture(GL_TEXTURE_2D, id);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // RGB 数据的行宽不一定是 4 的倍数，上传前显式设置像素对齐并在之后恢复。
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, 64, 64, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
        glGenerateMipmap(GL_TEXTURE_2D);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    }
    ~Texture() {
        glDeleteTextures(1, &id);
    }
    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;
    void bind(int unit = 0) const {
        glActiveTexture(GL_TEXTURE0 + unit);
        glBindTexture(GL_TEXTURE_2D, id);
    }
};
struct Camera {
    glm::vec3 pos{0, 1.2f, 6}, front{0, 0, -1};
    float yaw = -90, pitch = -8, fov = 45;
    double oldX = 0, oldY = 0;
    bool first = true;
    void update(App& a) {
        // 按住右键才捕获鼠标，松开后可以正常操作桌面。
        bool capture = glfwGetMouseButton(a.window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
        glfwSetInputMode(
            a.window, GLFW_CURSOR, capture ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
        double x, y;
        glfwGetCursorPos(a.window, &x, &y);
        if (capture && !first) {
            yaw += float(x - oldX) * .1f;
            pitch = std::clamp(pitch + float(oldY - y) * .1f, -89.f, 89.f);
        }
        first = !capture;
        oldX = x;
        oldY = y;
        front =
            glm::normalize(glm::vec3(std::cos(glm::radians(yaw)) * std::cos(glm::radians(pitch)),
                                     std::sin(glm::radians(pitch)),
                                     std::sin(glm::radians(yaw)) * std::cos(glm::radians(pitch))));

        // 运动速度乘以帧间隔，避免移动速度随帧率改变。
        float speed = 3.f * a.dt;
        auto right = glm::normalize(glm::cross(front, glm::vec3(0, 1, 0)));
        if (glfwGetKey(a.window, GLFW_KEY_W) == GLFW_PRESS)
            pos += front * speed;
        if (glfwGetKey(a.window, GLFW_KEY_S) == GLFW_PRESS)
            pos -= front * speed;
        if (glfwGetKey(a.window, GLFW_KEY_A) == GLFW_PRESS)
            pos -= right * speed;
        if (glfwGetKey(a.window, GLFW_KEY_D) == GLFW_PRESS)
            pos += right * speed;
        if (glfwGetKey(a.window, GLFW_KEY_Q) == GLFW_PRESS)
            pos.y -= speed;
        if (glfwGetKey(a.window, GLFW_KEY_E) == GLFW_PRESS)
            pos.y += speed;
    }
    glm::mat4 view() const {
        return glm::lookAt(pos, pos + front, glm::vec3(0, 1, 0));
    }
    glm::mat4 projection(const App& a) const {
        return glm::perspective(glm::radians(fov), a.aspect(), .1f, 100.f);
    }
};
inline int failure(const std::exception& e) {
    std::cerr << "ERROR: " << e.what() << '\n';
    return 1;
}
