#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <cmath>
#include <cstdint>
#include <cstring>
namespace course {

// 跨 API 共用的顶点布局：位置 12 字节、法线 12 字节、UV 8 字节。
// Vulkan attribute offsets 和 DX12 input layout 必须与这里一致。
struct Vertex {
    glm::vec3 position, normal;
    glm::vec2 uv;
};
static_assert(sizeof(Vertex) == 32);
struct Geometry {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
};
inline Geometry geometry(int mode) {
    Geometry g;
    if (mode == 4) {
        g.vertices = {{{-.7f, -.7f, 0}, {1, 0, 0}, {0, 0}},
                      {{.7f, -.7f, 0}, {0, 1, 0}, {1, 0}},
                      {{.7f, .7f, 0}, {0, 0, 1}, {1, 1}},
                      {{-.7f, -.7f, 0}, {1, 0, 0}, {0, 0}},
                      {{.7f, .7f, 0}, {0, 0, 1}, {1, 1}},
                      {{-.7f, .7f, 0}, {1, 1, 0}, {0, 1}}};
        return g;
    }
    if (mode >= 9) {
        int rings = 24, segments = 48;
        for (int y = 0; y <= rings; ++y)
            for (int x = 0; x <= segments; ++x) {
                float u = float(x) / segments, v = float(y) / rings;
                glm::vec3 n(std::sin(v * 3.14159265f) * std::cos(u * 6.2831853f),
                            std::cos(v * 3.14159265f),
                            std::sin(v * 3.14159265f) * std::sin(u * 6.2831853f));
                g.vertices.push_back({n, n, {u, v}});
            }
        for (int y = 0; y < rings; ++y)
            for (int x = 0; x < segments; ++x) {
                uint32_t a = uint32_t(y * (segments + 1) + x), b = a + segments + 1;
                g.indices.insert(g.indices.end(), {a, b, a + 1, a + 1, b, b + 1});
            }
        return g;
    }
    std::array<glm::vec3, 6> normals = {
        glm::vec3(0, 0, 1), {0, 0, -1}, {1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}};
    glm::vec3 p[6][4] = {{{-1, -1, 1}, {1, -1, 1}, {1, 1, 1}, {-1, 1, 1}},
                         {{1, -1, -1}, {-1, -1, -1}, {-1, 1, -1}, {1, 1, -1}},
                         {{1, -1, 1}, {1, -1, -1}, {1, 1, -1}, {1, 1, 1}},
                         {{-1, -1, -1}, {-1, -1, 1}, {-1, 1, 1}, {-1, 1, -1}},
                         {{-1, 1, 1}, {1, 1, 1}, {1, 1, -1}, {-1, 1, -1}},
                         {{-1, -1, -1}, {1, -1, -1}, {1, -1, 1}, {-1, -1, 1}}};
    glm::vec2 uv[4] = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};
    for (int f = 0; f < 6; ++f) {
        uint32_t start = uint32_t(g.vertices.size());
        for (int k = 0; k < 4; ++k)
            g.vertices.push_back({p[f][k] * .5f, normals[f], uv[k]});
        for (uint32_t k : {0u, 1u, 2u, 2u, 3u, 0u})
            g.indices.push_back(start + k);
    }
    return g;
}

// 每帧更新的相机矩阵；与 GLSL uniform / HLSL b0 的列主序矩阵对应。
struct alignas(16) Frame {
    glm::mat4 viewProjection{1};
};

// 每次绘制的数据：Vulkan 用 push constants，DX12 用 20 个 32 位 root constants。
// settings 分量依次为课程模式、金属度、粗糙度和时间。
struct alignas(16) Draw {
    glm::mat4 model{1};
    glm::vec4 settings{0};
};
static_assert(sizeof(Draw) == 80);
inline Frame frame(int mode, float aspect, bool vulkan) {
    Frame f;
    if (mode <= 4)
        return f;
    float distance = mode >= 9 ? 14.f : mode == 7 ? 10.f : 4.f;
    auto p = glm::perspectiveRH_ZO(glm::radians(45.f), aspect, .1f, 100.f);

    // 两条路线都使用 0..1 深度；Vulkan 正高度 viewport 额外翻转投影 Y。
    if (vulkan)
        p[1][1] *= -1;
    f.viewProjection = p * glm::lookAtRH(glm::vec3(0, mode >= 9 ? 0 : 1.5f, distance),
                                         glm::vec3(0),
                                         glm::vec3(0, 1, 0));
    return f;
}

// PBR 球阵列逐行改变金属度、逐列改变粗糙度，方便比较材质响应。
inline std::vector<Draw> draws(int mode, float time) {
    std::vector<Draw> d;
    if (mode >= 9) {
        for (int y = 0; y < 5; ++y)
            for (int x = 0; x < 7; ++x) {
                Draw a;
                a.model = glm::scale(
                    glm::translate(glm::mat4(1), glm::vec3((x - 3) * 1.65f, (y - 2) * 1.65f, 0)),
                    glm::vec3(.65f));
                a.settings = {float(mode), float(y) / 4, std::max(.06f, float(x) / 6), time};
                d.push_back(a);
            }
    } else {
        Draw a;
        a.settings = {float(mode), 0, .4f, time};
        if (mode >= 5)
            a.model = glm::rotate(glm::mat4(1), time * .5f, glm::vec3(.3f, 1, .1f));
        d.push_back(a);
    }
    return d;
}
inline std::vector<uint8_t> checker() {
    std::vector<uint8_t> b(64 * 64 * 4);
    for (int y = 0; y < 64; ++y)
        for (int x = 0; x < 64; ++x) {
            bool c = (x / 8 + y / 8) % 2 == 0;
            size_t i = size_t(y * 64 + x) * 4;
            b[i] = c ? 240 : 20;
            b[i + 1] = c ? 155 : 100;
            b[i + 2] = c ? 35 : 180;
            b[i + 3] = 255;
        }
    return b;
}
inline std::vector<uint32_t> spirv(const std::filesystem::path& path) {
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f)
        throw std::runtime_error("Missing shader: " + path.string());
    auto length = f.tellg();
    if (length <= 0 || size_t(length) % 4)
        throw std::runtime_error("Invalid SPIR-V length");
    std::vector<uint32_t> data(size_t(length) / 4);
    f.seekg(0);
    f.read(reinterpret_cast<char*>(data.data()), length);
    return data;
}

// 读回检查既保存图像，也检查颜色范围，避免把空白帧当成渲染成功。
inline void capture(const std::filesystem::path& path,
                    const uint8_t* rgba,
                    int w,
                    int h,
                    bool bgra = false,
                    bool requireGeometry = true) {
    int lo = 255, hi = 0;
    std::ofstream f(path, std::ios::binary);
    f << "P6\n" << w << ' ' << h << "\n255\n";
    for (int i = 0; i < w * h; ++i) {
        char rgb[3] = {char(rgba[i * 4 + (bgra ? 2 : 0)]),
                       char(rgba[i * 4 + 1]),
                       char(rgba[i * 4 + (bgra ? 0 : 2)])};
        for (char c : rgb) {
            lo = std::min(lo, int(uint8_t(c)));
            hi = std::max(hi, int(uint8_t(c)));
        }
        f.write(rgb, 3);
    }
    if (requireGeometry && hi - lo < 30)
        throw std::runtime_error("Readback is blank");
    std::cout << "GPU readback: " << path.filename() << ", range " << lo << ".." << hi << '\n';
}
} // namespace course
