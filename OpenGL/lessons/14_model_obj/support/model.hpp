#pragma once
#include "advanced.hpp"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/material.h>
#include <glm/gtc/quaternion.hpp>

// Assimp 矩阵按行给出元素，转置后匹配 GLM 的列主序存储。
inline glm::mat4 convert(const aiMatrix4x4& a) {
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
struct ImportedMesh {
    struct Vertex {
        glm::vec3 position, normal;
        glm::vec2 uv;
        glm::ivec4 joints{0};
        glm::vec4 weights{0};
    };
    GLuint vao = 0, vbo = 0, ebo = 0;
    GLsizei count;
    unsigned material;
    std::string node;
    std::vector<std::string> bones;
    std::vector<glm::mat4> offsets;
    ImportedMesh(const aiMesh& mesh, const std::string& owner)
        : count(0), material(mesh.mMaterialIndex), node(owner) {
        if (mesh.mNumBones > 48)
            throw std::runtime_error("Teaching skin supports at most 48 bones per mesh");
        std::vector<Vertex> vertices(mesh.mNumVertices);
        for (unsigned i = 0; i < mesh.mNumVertices; ++i) {
            auto p = mesh.mVertices[i];
            auto n = mesh.HasNormals() ? mesh.mNormals[i] : aiVector3D(0, 1, 0);
            auto t = mesh.HasTextureCoords(0) ? mesh.mTextureCoords[0][i] : aiVector3D(0);
            vertices[i].position = {p.x, p.y, p.z};
            vertices[i].normal = {n.x, n.y, n.z};
            vertices[i].uv = {t.x, t.y};
        }
        // 每顶点保留最强的四个权重，再归一化；不要简单丢弃后来的大权重。
        std::vector<std::vector<std::pair<float, int>>> influences(mesh.mNumVertices);
        for (unsigned b = 0; b < mesh.mNumBones; ++b) {
            auto bone = mesh.mBones[b];
            bones.emplace_back(bone->mName.C_Str());
            offsets.push_back(convert(bone->mOffsetMatrix));
            for (unsigned k = 0; k < bone->mNumWeights; ++k) {
                auto w = bone->mWeights[k];
                if (w.mVertexId >= vertices.size())
                    throw std::runtime_error("Invalid vertex weight");
                influences[w.mVertexId].emplace_back(w.mWeight, int(b));
            }
        }
        for (size_t i = 0; i < vertices.size(); ++i) {
            auto& ws = influences[i];
            std::sort(ws.begin(), ws.end(), [](auto a, auto b) {
                return a.first > b.first;
            });
            float total = 0;
            for (size_t k = 0; k < std::min(size_t(4), ws.size()); ++k) {
                vertices[i].joints[int(k)] = ws[k].second;
                vertices[i].weights[int(k)] = ws[k].first;
                total += ws[k].first;
            }
            if (total > 0)
                vertices[i].weights /= total;
        }
        std::vector<unsigned> indices;
        for (unsigned f = 0; f < mesh.mNumFaces; ++f) {
            const auto& face = mesh.mFaces[f];
            if (face.mNumIndices == 3)
                for (unsigned k = 0; k < 3; ++k)
                    indices.push_back(face.mIndices[k]);
        }
        count = GLsizei(indices.size());
        if (!count)
            throw std::runtime_error("Imported mesh has no triangles");
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glGenBuffers(1, &ebo);
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER,
                     GLsizeiptr(vertices.size() * sizeof(Vertex)),
                     vertices.data(),
                     GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                     GLsizeiptr(indices.size() * sizeof(unsigned)),
                     indices.data(),
                     GL_STATIC_DRAW);

        // 骨骼索引使用整数 attribute；不能用会转换成浮点数的 glVertexAttribPointer。
        const size_t offsetsBytes[] = {offsetof(Vertex, position),
                                       offsetof(Vertex, normal),
                                       offsetof(Vertex, uv),
                                       offsetof(Vertex, joints),
                                       offsetof(Vertex, weights)};
        for (GLuint i = 0; i < 5; ++i) {
            glEnableVertexAttribArray(i);
            if (i == 3)
                glVertexAttribIPointer(
                    i, 4, GL_INT, sizeof(Vertex), reinterpret_cast<void*>(offsetsBytes[i]));
            else
                glVertexAttribPointer(i,
                                      i == 2 ? 2 : (i == 4 ? 4 : 3),
                                      GL_FLOAT,
                                      GL_FALSE,
                                      sizeof(Vertex),
                                      reinterpret_cast<void*>(offsetsBytes[i]));
        }
        glBindVertexArray(0);
    }
    ~ImportedMesh() {
        glDeleteBuffers(1, &ebo);
        glDeleteBuffers(1, &vbo);
        glDeleteVertexArrays(1, &vao);
    }
    ImportedMesh(const ImportedMesh&) = delete;
    ImportedMesh& operator=(const ImportedMesh&) = delete;
    void draw() const {
        glBindVertexArray(vao);
        // 绘制使用当前程序、VAO 和附件；属性布局必须匹配顶点着色器。
        glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, nullptr);
    }
};
struct ImportedMaterial {
    glm::vec3 color{1};
    float metallic = 0, roughness = .5f;
    std::shared_ptr<ImageTexture> albedo, normal, mr, ao;
    bool hasNormal = false;
};
struct Model {

    // aiScene 的内存由 Importer 持有，Importer 必须覆盖整个模型使用期。
    Assimp::Importer importer;
    const aiScene* scene = nullptr;
    std::filesystem::path directory;
    std::vector<std::unique_ptr<ImportedMesh>> meshes;
    std::vector<ImportedMaterial> materials;
    std::map<std::string, std::shared_ptr<ImageTexture>> textureCache;
    std::map<std::string, glm::mat4> globals;
    Model(const std::filesystem::path& path) : directory(path.parent_path()) {

        // 先三角化并补齐法线，再把场景拆成网格、材质和节点层级。
        scene = importer.ReadFile(path.string(),
                                  aiProcess_Triangulate | aiProcess_GenSmoothNormals |
                                      aiProcess_CalcTangentSpace | aiProcess_JoinIdenticalVertices |
                                      aiProcess_ValidateDataStructure);
        if (!scene || !scene->mRootNode || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE))
            throw std::runtime_error(importer.GetErrorString());
        for (unsigned i = 0; i < scene->mNumMaterials; ++i) {
            auto m = scene->mMaterials[i];
            ImportedMaterial material;
            aiColor4D c(1, 1, 1, 1);
            if (m->Get(AI_MATKEY_BASE_COLOR, c) != AI_SUCCESS)
                m->Get(AI_MATKEY_COLOR_DIFFUSE, c);
            material.color = {c.r, c.g, c.b};
            m->Get(AI_MATKEY_METALLIC_FACTOR, material.metallic);
            m->Get(AI_MATKEY_ROUGHNESS_FACTOR, material.roughness);
            material.albedo = loadTexture(
                *m, aiTextureType_BASE_COLOR, true, {255, 255, 255, 255}, aiTextureType_DIFFUSE);
            material.normal = loadTexture(*m, aiTextureType_NORMALS, false, {128, 128, 255, 255});
            material.hasNormal = m->GetTextureCount(aiTextureType_NORMALS) > 0;
            material.mr = loadTexture(*m, aiTextureType_METALNESS, false, {255, 255, 255, 255});
            material.ao =
                loadTexture(*m, aiTextureType_AMBIENT_OCCLUSION, false, {255, 255, 255, 255});
            materials.push_back(std::move(material));
        }
        gather(scene->mRootNode);
        update(0);
        std::cout << "Imported " << path.filename() << ": " << meshes.size() << " mesh(es), "
                  << materials.size() << " material(s), " << scene->mNumAnimations
                  << " animation(s)\n";
    }
    std::shared_ptr<ImageTexture> loadTexture(const aiMaterial& mat,
                                              aiTextureType type,
                                              bool srgb,
                                              std::array<unsigned char, 4> fallback,
                                              aiTextureType alternative = aiTextureType_NONE) {
        aiString p;
        if (mat.GetTexture(type, 0, &p) != AI_SUCCESS &&
            (alternative == aiTextureType_NONE || mat.GetTexture(alternative, 0, &p) != AI_SUCCESS))
            return std::make_shared<ImageTexture>(fallback, srgb);

        // 同一图片的 sRGB 与线性用途需要不同缓存项，避免颜色空间被错误复用。
        std::string key = std::string(p.C_Str()) + (srgb ? "#srgb" : "#linear");
        auto old = textureCache.find(key);
        if (old != textureCache.end())
            return old->second;
        std::shared_ptr<ImageTexture> texture;

        // glTF 等格式可能把图片嵌入模型；压缩图片和原始像素分别解码。
        if (const aiTexture* embedded = scene->GetEmbeddedTexture(p.C_Str())) {
            if (embedded->mHeight == 0)
                texture = std::make_shared<ImageTexture>(
                    reinterpret_cast<const unsigned char*>(embedded->pcData),
                    int(embedded->mWidth),
                    srgb);
            else {
                std::vector<unsigned char> rgba(size_t(embedded->mWidth) * embedded->mHeight * 4);
                for (size_t k = 0; k < rgba.size() / 4; ++k) {
                    auto v = embedded->pcData[k];
                    rgba[k * 4] = v.r;
                    rgba[k * 4 + 1] = v.g;
                    rgba[k * 4 + 2] = v.b;
                    rgba[k * 4 + 3] = v.a;
                }
                texture = std::make_shared<ImageTexture>(
                    std::array<unsigned char, 4>{255, 255, 255, 255});
                texture->upload(int(embedded->mWidth), int(embedded->mHeight), rgba.data(), srgb);
            }
        } else {
            std::string relative = p.C_Str();
            std::replace(relative.begin(), relative.end(), '\\', '/');
            texture =
                std::make_shared<ImageTexture>(directory / std::filesystem::path(relative), srgb);
        }
        textureCache[key] = texture;
        return texture;
    }
    void gather(const aiNode* n) {
        for (unsigned k = 0; k < n->mNumMeshes; ++k)
            meshes.push_back(
                std::make_unique<ImportedMesh>(*scene->mMeshes[n->mMeshes[k]], n->mName.C_Str()));
        for (unsigned k = 0; k < n->mNumChildren; ++k)
            gather(n->mChildren[k]);
    }
    static aiVector3D
    interpolate(const aiVectorKey* keys, unsigned count, double t, const aiVector3D& fallback) {
        if (!count)
            return fallback;
        if (count == 1 || t <= keys[0].mTime)
            return keys[0].mValue;
        unsigned i = 0;
        while (i + 1 < count && t > keys[i + 1].mTime)
            ++i;
        if (i + 1 == count)
            return keys[i].mValue;
        float f = float((t - keys[i].mTime) / (keys[i + 1].mTime - keys[i].mTime));
        return keys[i].mValue * (1 - f) + keys[i + 1].mValue * f;
    }

    // 动画秒数先换算成 tick 并循环，再沿节点树累积父子变换。
    void update(float seconds) {
        double tick = 0;
        const aiAnimation* animation = nullptr;
        if (scene->mNumAnimations) {
            animation = scene->mAnimations[0];
            double rate = animation->mTicksPerSecond ? animation->mTicksPerSecond : 25;
            tick = std::fmod(seconds * rate, std::max(animation->mDuration, .001));
        }
        visit(scene->mRootNode, glm::mat4(1), animation, tick);
    }
    void
    visit(const aiNode* n, const glm::mat4& parent, const aiAnimation* animation, double tick) {
        glm::mat4 local = convert(n->mTransformation);
        if (animation)
            for (unsigned k = 0; k < animation->mNumChannels; ++k) {
                auto ch = animation->mChannels[k];
                if (ch->mNodeName != n->mName)
                    continue;
                aiVector3D scale, position;
                aiQuaternion rot;
                n->mTransformation.Decompose(scale, rot, position);
                position = interpolate(ch->mPositionKeys, ch->mNumPositionKeys, tick, position);
                scale = interpolate(ch->mScalingKeys, ch->mNumScalingKeys, tick, scale);
                if (ch->mNumRotationKeys) {
                    unsigned j = 0;
                    while (j + 1 < ch->mNumRotationKeys && tick > ch->mRotationKeys[j + 1].mTime)
                        ++j;
                    rot = ch->mRotationKeys[j].mValue;
                    if (j + 1 < ch->mNumRotationKeys) {
                        float f =
                            float((tick - ch->mRotationKeys[j].mTime) /
                                  (ch->mRotationKeys[j + 1].mTime - ch->mRotationKeys[j].mTime));
                        aiQuaternion::Interpolate(rot, rot, ch->mRotationKeys[j + 1].mValue, f);
                        rot.Normalize();
                    }
                }
                local =
                    glm::translate(glm::mat4(1), glm::vec3(position.x, position.y, position.z)) *
                    glm::mat4_cast(glm::quat(rot.w, rot.x, rot.y, rot.z)) *
                    glm::scale(glm::mat4(1), glm::vec3(scale.x, scale.y, scale.z));
                break;
            }
        auto global = parent * local;
        globals[n->mName.C_Str()] = global;
        for (unsigned k = 0; k < n->mNumChildren; ++k)
            visit(n->mChildren[k], global, animation, tick);
    }
    void draw(const Shader& s, const glm::mat4& rootTransform = glm::mat4(1)) const {
        for (const auto& mesh : meshes) {
            auto global = globals.at(mesh->node);
            s.set("model", rootTransform * global);
            s.set("skinned", !mesh->bones.empty());

            // 骨骼矩阵先转回网格局部空间，再乘 inverse bind pose，供顶点蒙皮使用。
            for (size_t b = 0; b < mesh->bones.size(); ++b) {
                auto found = globals.find(mesh->bones[b]);
                if (found == globals.end())
                    throw std::runtime_error("Bone node missing");
                s.set(("bones[" + std::to_string(b) + "]").c_str(),
                      glm::inverse(global) * found->second * mesh->offsets[b]);
            }
            const auto& mat = materials.at(mesh->material);
            mat.albedo->bind(0);
            mat.normal->bind(1);
            mat.mr->bind(2);
            mat.ao->bind(3);
            s.set("baseColor", mat.color);
            s.set("metallic", mat.metallic);
            s.set("roughness", mat.roughness);
            s.set("useMaps", 1);
            s.set("useTexture", 1);
            s.set("useNormal", mat.hasNormal);
            mesh->draw();
        }
    }
};
