#pragma once
#include "scene.hpp"
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <stb_image.h>
#include <algorithm>
#include <functional>
#include <limits>

namespace course {
inline void require(bool condition, const char* message) {
    if (!condition)
        throw std::runtime_error(message);
}
inline uint64_t alignUp(uint64_t value, uint64_t alignment) {
    require(alignment != 0, "Alignment must be nonzero");
    return ((value + alignment - 1) / alignment) * alignment;
}
struct Pixels {
    uint32_t width{}, height{};
    std::vector<uint8_t> rgba;
};
inline Pixels imageFile(const std::filesystem::path& path) {
    int w{}, h{}, channels{};
    auto* bytes = stbi_load(path.string().c_str(), &w, &h, &channels, 4);
    require(bytes != nullptr, "Cannot decode course image");
    Pixels result{uint32_t(w), uint32_t(h), {}};
    result.rgba.assign(bytes, bytes + size_t(w) * h * 4);
    stbi_image_free(bytes);
    return result;
}
// CPU mip 参考：UNORM 按数值平均；sRGB 的 RGB 先解码到线性再平均和编码，alpha 始终线性。
inline std::vector<Pixels> mipReference(Pixels base, bool srgb = false) {
    std::vector<Pixels> levels;
    levels.push_back(std::move(base));
    while (levels.back().width > 1 || levels.back().height > 1) {
        const auto& previous = levels.back();
        Pixels next{std::max(1u, previous.width / 2), std::max(1u, previous.height / 2), {}};
        next.rgba.resize(size_t(next.width) * next.height * 4);
        for (uint32_t y = 0; y < next.height; ++y)
            for (uint32_t x = 0; x < next.width; ++x)
                for (uint32_t c = 0; c < 4; ++c) {
                    uint32_t sum = 0;
                    float linearSum = 0;
                    for (uint32_t dy = 0; dy < 2; ++dy)
                        for (uint32_t dx = 0; dx < 2; ++dx) {
                            auto sx = std::min(x * 2 + dx, previous.width - 1);
                            auto sy = std::min(y * 2 + dy, previous.height - 1);
                            auto encoded =
                                previous.rgba[(size_t(sy) * previous.width + sx) * 4 + c];
                            sum += encoded;
                            float value = encoded / 255.f;
                            linearSum += value <= .04045f
                                             ? value / 12.92f
                                             : std::pow((value + .055f) / 1.055f, 2.4f);
                        }
                    next.rgba[(size_t(y) * next.width + x) * 4 + c] = uint8_t((sum + 2) / 4);
                    if (srgb && c < 3) {
                        float linear = linearSum * .25f;
                        float encoded = linear <= .0031308f
                                            ? linear * 12.92f
                                            : 1.055f * std::pow(linear, 1.f / 2.4f) - .055f;
                        next.rgba[(size_t(y) * next.width + x) * 4 + c] =
                            uint8_t(std::clamp(std::lround(encoded * 255), 0l, 255l));
                    }
                }
        levels.push_back(std::move(next));
    }
    return levels;
}
inline glm::mat4 assimpMatrix(const aiMatrix4x4& a) {
    return glm::transpose(glm::mat4(a.a1,
                                    a.a2,
                                    a.a3,
                                    a.a4,
                                    a.b1,
                                    a.b2,
                                    a.b3,
                                    a.b4,
                                    a.c1,
                                    a.c2,
                                    a.c3,
                                    a.c4,
                                    a.d1,
                                    a.d2,
                                    a.d3,
                                    a.d4));
}
// Traverse node transforms before flattening: imported mesh positions are node-local.
// This lab deliberately flattens a static pose; full materials/skin animation are separate work.
inline Geometry staticAsset(const std::filesystem::path& path) {
    Assimp::Importer importer;
    const auto* scene =
        importer.ReadFile(path.string(),
                          aiProcess_Triangulate | aiProcess_GenSmoothNormals |
                              aiProcess_JoinIdenticalVertices | aiProcess_ValidateDataStructure);
    require(scene && scene->mRootNode, "Assimp could not load course model");
    Geometry result;
    unsigned nodes = 0;
    std::function<void(const aiNode*, glm::mat4)> visit = [&](const aiNode* node,
                                                              glm::mat4 parent) {
        ++nodes;
        glm::mat4 transform = parent * assimpMatrix(node->mTransformation);
        glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(transform)));
        for (unsigned m = 0; m < node->mNumMeshes; ++m) {
            const auto* mesh = scene->mMeshes[node->mMeshes[m]];
            uint32_t base = uint32_t(result.vertices.size());
            for (unsigned i = 0; i < mesh->mNumVertices; ++i) {
                const auto& p = mesh->mVertices[i];
                auto n = mesh->HasNormals() ? mesh->mNormals[i] : aiVector3D(0, 1, 0);
                auto uv = mesh->HasTextureCoords(0) ? mesh->mTextureCoords[0][i] : aiVector3D();
                result.vertices.push_back({glm::vec3(transform * glm::vec4(p.x, p.y, p.z, 1)),
                                           glm::normalize(normalMatrix * glm::vec3(n.x, n.y, n.z)),
                                           {uv.x, uv.y}});
            }
            for (unsigned f = 0; f < mesh->mNumFaces; ++f) {
                require(mesh->mFaces[f].mNumIndices == 3, "Non-triangle after triangulation");
                for (unsigned k = 0; k < 3; ++k)
                    result.indices.push_back(base + mesh->mFaces[f].mIndices[k]);
            }
        }
        for (unsigned i = 0; i < node->mNumChildren; ++i)
            visit(node->mChildren[i], transform);
    };
    visit(scene->mRootNode, glm::mat4(1));
    require(!result.vertices.empty() && !result.indices.empty(), "Empty imported model");
    glm::vec3 lo(std::numeric_limits<float>::max()), hi(-std::numeric_limits<float>::max());
    for (const auto& v : result.vertices) {
        lo = glm::min(lo, v.position);
        hi = glm::max(hi, v.position);
    }
    auto extent = hi - lo;
    float scale = 2.f / std::max({extent.x, extent.y, extent.z, .0001f});
    for (auto& v : result.vertices)
        v.position = (v.position - (hi + lo) * .5f) * scale;
    std::cout << "Imported nodes=" << nodes << ", meshes=" << scene->mNumMeshes
              << ", vertices=" << result.vertices.size() << ", indices=" << result.indices.size()
              << '\n';
    return result;
}
inline Geometry particleGeometry() {
    Geometry g;
    g.vertices.resize(96 * 3); // GPU fills three vertices per particle.
    return g;
}
struct LabOptions {
    std::filesystem::path directory;
    bool smoke = false, warp = false;
    int id = 0;
};
inline LabOptions labOptions(int argc, char** argv, int id) {
    LabOptions result{
        std::filesystem::absolute(argv[0]).parent_path() / COURSE_RUNTIME_NAME, false, false, id};
    for (int i = 1; i < argc; ++i) {
        result.smoke |= std::strcmp(argv[i], "--smoke") == 0;
        result.warp |= std::strcmp(argv[i], "--warp") == 0;
    }
    return result;
}
} // namespace course
