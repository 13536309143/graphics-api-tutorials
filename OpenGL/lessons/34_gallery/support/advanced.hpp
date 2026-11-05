#pragma once
#include "app.hpp"
#include <stb_image.h>
#include <array>
#include <memory>
#include <random>
#include <map>

inline std::filesystem::path asset(const App& a, const std::string& p) {
    return a.assets.parent_path() / "assets" / p;
}

// 颜色贴图可使用 sRGB 内部格式；法线、粗糙度等数据贴图必须在线性空间采样。
struct ImageTexture {
    GLuint id = 0;
    ImageTexture(const std::filesystem::path& path, bool srgb = false) {
        int w, h, c;
        auto bytes = stbi_load(path.string().c_str(), &w, &h, &c, 4);
        if (!bytes)
            throw std::runtime_error("Image load failed: " + path.string());
        upload(w, h, bytes, srgb);
        stbi_image_free(bytes);
    }
    ImageTexture(const unsigned char* memory, int length, bool srgb) {
        int w, h, c;
        auto bytes = stbi_load_from_memory(memory, length, &w, &h, &c, 4);
        if (!bytes)
            throw std::runtime_error("Embedded image decode failed");
        upload(w, h, bytes, srgb);
        stbi_image_free(bytes);
    }
    explicit ImageTexture(std::array<unsigned char, 4> rgba, bool srgb = false) {
        upload(1, 1, rgba.data(), srgb);
    }
    void upload(int w, int h, const unsigned char* bytes, bool srgb) {
        if (!id)
            glGenTextures(1, &id);
        glBindTexture(GL_TEXTURE_2D, id);
        glTexImage2D(GL_TEXTURE_2D,
                     0,
                     srgb ? GL_SRGB8_ALPHA8 : GL_RGBA8,
                     w,
                     h,
                     0,
                     GL_RGBA,
                     GL_UNSIGNED_BYTE,
                     bytes);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    ~ImageTexture() {
        glDeleteTextures(1, &id);
    }
    ImageTexture(const ImageTexture&) = delete;
    ImageTexture& operator=(const ImageTexture&) = delete;
    void bind(int unit = 0) const {
        glActiveTexture(GL_TEXTURE0 + unit);
        glBindTexture(GL_TEXTURE_2D, id);
    }
};

// 离屏目标可带多个颜色附件；HDR 使用浮点纹理，保留超过 1 的光照能量。
struct RenderTarget {
    GLuint fbo = 0, depth = 0;
    std::vector<GLuint> colors;
    int width = 0, height = 0;
    bool hdr;
    RenderTarget(int attachments = 1, bool floating = true)
        : colors(size_t(attachments)), hdr(floating) {
        glGenFramebuffers(1, &fbo);
        glGenRenderbuffers(1, &depth);
        glGenTextures(attachments, colors.data());
    }
    ~RenderTarget() {
        glDeleteTextures(GLsizei(colors.size()), colors.data());
        glDeleteRenderbuffers(1, &depth);
        glDeleteFramebuffers(1, &fbo);
    }
    RenderTarget(const RenderTarget&) = delete;
    RenderTarget& operator=(const RenderTarget&) = delete;
    void resize(int w, int h) {
        if (w == width && h == height)
            return;
        width = w;
        height = h;
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        std::vector<GLenum> draw;
        for (size_t i = 0; i < colors.size(); ++i) {
            glBindTexture(GL_TEXTURE_2D, colors[i]);
            glTexImage2D(
                GL_TEXTURE_2D, 0, hdr ? GL_RGBA16F : GL_RGBA8, w, h, 0, GL_RGBA, GL_FLOAT, nullptr);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            GLenum attachment = GL_COLOR_ATTACHMENT0 + GLenum(i);
            draw.push_back(attachment);
            glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D, colors[i], 0);
        }

        // MRT 需要显式列出片元着色器输出要写入的颜色附件。
        // 绘制使用当前程序、VAO 和附件；属性布局必须匹配顶点着色器。
        glDrawBuffers(GLsizei(draw.size()), draw.data());
        glBindRenderbuffer(GL_RENDERBUFFER, depth);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, w, h);
        glFramebufferRenderbuffer(
            GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depth);
        check();
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
    static void check() {
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            throw std::runtime_error("Advanced FBO incomplete");
    }
    void bind() const {
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glViewport(0, 0, width, height);
    }
};
inline void bind2D(GLuint texture, int unit) {
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, texture);
}
inline void bindCube(GLuint texture, int unit) {
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_CUBE_MAP, texture);
}

// 返回默认 framebuffer 时也恢复 viewport，避免沿用离屏目标的尺寸。
inline void defaultTarget(const App& a) {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, a.width, a.height);
}
inline void cameraUniforms(const Shader& s, const App& a, const Camera& c) {
    s.set("view", c.view());
    s.set("projection", c.projection(a));
    s.set("eye", c.pos);
}
inline std::vector<float> sphere(int segments = 48, int rings = 24) {
    std::vector<float> v;
    auto add = [&](int x, int y) {
        float u = float(x) / segments, t = float(y) / rings;
        float phi = u * 6.2831853f, theta = t * 3.14159265f;
        glm::vec3 n(
            std::sin(theta) * std::cos(phi), std::cos(theta), std::sin(theta) * std::sin(phi));
        v.insert(v.end(), {n.x, n.y, n.z, n.x, n.y, n.z, u, t});
    };
    for (int y = 0; y < rings; ++y)
        for (int x = 0; x < segments; ++x) {
            add(x, y);
            add(x + 1, y);
            add(x + 1, y + 1);
            add(x, y);
            add(x + 1, y + 1);
            add(x, y + 1);
        }
    return v;
}
inline void surfaceDefaults(const Shader& s) {
    s.set("baseColor", glm::vec3(.9f, .35f, .12f));
    s.set("light", glm::vec3(-3, 4, 5));
    s.set("opacity", 1.f);
    s.set("albedoMap", 0);
    s.set("normalMap", 1);
    s.set("heightMap", 2);
    s.set("useTexture", 0);
    s.set("useNormal", 0);
    s.set("useParallax", 0);
    s.set("heightScale", .07f);
}
inline void pbrDefaults(const Shader& s) {
    s.set("albedoMap", 0);
    s.set("normalMap", 1);
    s.set("mrMap", 2);
    s.set("aoMap", 3);
    s.set("irradianceMap", 4);
    s.set("prefilterMap", 5);
    s.set("brdfLUT", 6);
    s.set("baseColor", glm::vec3(.82f, .23f, .07f));
    s.set("metallic", 0.f);
    s.set("roughness", .4f);
    s.set("useMaps", 0);
    s.set("useNormal", 0);
    s.set("useIBL", 0);
    s.set("displayEncode", 1);
}

// 附加几何着色器后必须重新链接，并重新检查链接结果。
inline void geometryStage(Shader& program, const App& a, const char* name) {
    GLuint g = Shader::compile(GL_GEOMETRY_SHADER, Shader::read(a.assets / name));
    glAttachShader(program.id, g);
    glLinkProgram(program.id);
    glDeleteShader(g);
    GLint ok;
    glGetProgramiv(program.id, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[4096];
        glGetProgramInfoLog(program.id, sizeof(log), nullptr, log);
        throw std::runtime_error(log);
    }
}
