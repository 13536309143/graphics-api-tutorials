# 11 · 模型、材质、节点与骨骼：共享什么，重写什么

[对照路线](../COMPARE.md) · [交互式对照](index.html#11_models_animation) · [上一章](10_compute_readback.md) · [下一章](12_lighting_pbr_ibl.md)

模型加载不是某个图形 API 的专用命令。文件解析得到 CPU 场景，后端再把网格、图片和材质参数转换成 GPU 资源。将这两层分开，才有可能把 OpenGL 的模型功能迁到 Vulkan/DX12。

## 当前真实实现

OpenGL 14/15 通过 Assimp 读取 OBJ/glTF，31 演示骨骼动画，34 将模型与材质/环境组合。Vulkan/DX12 新增 21/22，实际导入 OBJ 和 glTF 静态节点，递归应用层级变换并绘制。完整 glTF 材质/skin/动画尚未迁移，下面的骨骼与完整资产方案继续作为工程设计。

## 导入后不能只取一个 vertices 数组

场景通常有多个 mesh、material、node；同一 mesh 还可能被多个节点实例化。几何局部坐标需要节点层级累积变换。材质可能引用外部图片、嵌入压缩图片或原始像素，还要区分 sRGB 颜色贴图与线性数据贴图。

OpenGL `Model` 持有 Assimp::Importer，因此 aiScene 在使用期间有效。`gather` 收集网格及所属节点，`visit` 递归累计变换，draw 时把 mesh 对应节点的 global 与外部 rootTransform 合成。绕过节点树直接画 mesh，可能得到重叠、位置错误或动画完全失效。

| 可共享层 | 内容 | 各后端要实现的部分 |
| --- | --- | --- |
| 文件解析 | Assimp、路径、嵌入图片 | 通常无需按 API 重写 |
| CPU 场景 | 节点、材质、网格、动画通道 | 统一自己的数据结构 |
| GPU 网格 | 顶点/索引及布局 | VAO；VkBuffer+layout；VBV/IBV |
| GPU 材质 | 纹理、采样器、参数 | texture unit；descriptor；SRV/root binding |
| 绘制调度 | 按材质组织、实例与动画 | 命令录制及同步 |

## 骨骼矩阵的空间关系

顶点带最多四个 joint index 和 weight。当前代码先收集影响，保留最强四项再归一化，避免简单保留最早四项丢掉重要骨骼。未受骨骼影响的顶点必须有明确处理，不能把全零权重矩阵直接作为正常变换。

当前 draw 使用 `inverse(meshGlobal) * boneGlobal * offset` 转回网格局部空间，再由 shader 混合骨骼矩阵。offset 表示绑定姿态的逆变换，不是“当前骨骼位置”。CPU 动画将秒换为 tick、插值位置/缩放/旋转，再沿层级组合；四元数插值后归一化，避免旋转矩阵漂移。

## 分四步迁移，逐步验收

第一、二步已由新实验落实：导入静态 mesh、法线/UV，并支持节点 global transform 和多个 mesh。第三步加入纹理与材质，检查缓存键是否区分 sRGB/linear。第四步才扩展 GPU 顶点布局、joint buffer 和每帧骨骼更新。

Vulkan 的骨骼数组可由 UBO 或 storage buffer 提供，选择取决于数量与设备限制；DX12 可用常量或结构化缓冲。不要因当前 Draw 是 80 字节就把整套骨骼塞进 push/root constants。每帧更新骨骼后仍需保护在途 GPU 访问。

## 实验与故障定位

先运行静态 glTF，再运行骨骼课，逐一检查节点名、mesh 数、material index、骨骼数和权重和。模型变成刺猬时先查 joint index 类型、stride 和矩阵空间；贴图正常但位置错，先查节点层级；静态正确动画错，查时间单位、通道匹配和 inverse bind pose。

自测：为什么不能把 OpenGL ImportedMesh 整个类直接换名为 VkMesh？答案：它把 CPU 场景信息和 VAO/GL 状态放在一起。应先分离数据，再为后端实现上传、绑定和生命周期，而不是替换函数名前缀。

<!-- generated-source-comparison -->

## 与本仓库实现并排核对

以下切片直接取自当前仓库，省略了前后文。行号用于定位，不能把片段当作完整可编译程序。

| 路线 | 可运行参照 / 范围 |
| --- | --- |
| OpenGL | [31_skeletal](../../OpenGL/lessons/31_skeletal/README.md) |
| Vulkan 原生 | [vk22_gltf_scene](../../Vulkan/native/lessons/vk22_gltf_scene/README.md) |
| Vulkan / nvpro_core2 | 迁移设计 |
| DirectX 12 | [dx22_gltf_scene](../../DirectX12/lessons/dx22_gltf_scene/README.md) |

### OpenGL · 已实现

按 mesh 节点变换与 inverse bind pose 构造局部蒙皮矩阵。

[OpenGL/lessons/31_skeletal/support/model.hpp](../../OpenGL/lessons/31_skeletal/support/model.hpp#L305)，第 305—333 行：

```cpp
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
```

### Vulkan 原生 · 静态模型已实现

真实静态 glTF 节点递归与几何绘制；完整材质、skin/动画仍待迁移。

[Vulkan/native/lessons/vk22_gltf_scene/support/advanced.hpp](../../Vulkan/native/lessons/vk22_gltf_scene/support/advanced.hpp#L92)，第 92—127 行：

```cpp
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
```

### Vulkan / nvpro_core2 · 迁移设计

框架有资产相关辅助，但本课程未接入完整 glTF/skin；需要单独实现并验收。


### DirectX 12 · 静态模型已实现

真实静态 glTF 节点递归与几何绘制；完整材质、skin/动画仍待迁移。

[DirectX12/lessons/dx22_gltf_scene/support/advanced.hpp](../../DirectX12/lessons/dx22_gltf_scene/support/advanced.hpp#L92)，第 92—127 行：

```cpp
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
```
