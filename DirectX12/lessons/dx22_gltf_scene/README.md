# dx22_gltf_scene · glTF 节点层级

[54 课目录](../../COURSE.md)

## 目的与实际执行

使用相同 CPU 场景结果，后端差异集中于资源与管线。这里使用教学纹理/光照，不宣称完整还原 glTF 的每个材质扩展、透明模式或动画插值。

本程序有独立实验选择与验证路径，公共实现集中在 advanced_labs.cpp。设备/API 检查、数值 compute 与图形渲染采用不同证据；不是每个程序都会弹出窗口。

## 构建和运行

```powershell
cmake --preset dx12
cmake --build --preset dx12-debug --target dx22_gltf_scene --parallel 4
.\DirectX12\build\bin\Debug\dx22_gltf_scene.exe
```

## 深入学习：数据、接口与执行

- [第 42 课：glTF 节点层级、法线与静态姿态](../../chapters/42_gltf_nodes/README.md)

### 本课的真实源码切片

来自 [shared/advanced.hpp](../../../shared/advanced.hpp#L92)，第 92—135 行；这是现有实现的局部摘录，末尾可能处于函数中间，不是可单独编译的完整程序。

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
    glm::vec3 lo(std::numeric_limits<float>::max()), hi(-std::numeric_limits<float>::max());
    for (const auto& v : result.vertices) {
        lo = glm::min(lo, v.position);
        hi = glm::max(hi, v.position);
    }
    auto extent = hi - lo;
    float scale = 2.f / std::max({extent.x, extent.y, extent.z, .0001f});
    for (auto& v : result.vertices)
```

### 同主题跨 API 导航

| 路线 | 程序 |
| --- | --- |
| DirectX 12 | [dx22_gltf_scene](README.md) |
| 对照后端 | [vk22_gltf_scene](../../../Vulkan/native/lessons/vk22_gltf_scene/README.md) |

### 先预测，再验证

给 glTF 的父节点增加平移，预测所有后代是否一起移动，并说明法线如何处理非均匀缩放。

参考推导：后代 global 包含父变换，因此一起移动；法线使用线性部分的逆转置。单纯把 mesh 放原点会破坏层级关系。

## 验证与边界

运行 `--smoke` 执行有限自动检查。数值实验读取实际 GPU 结果，图形实验检查非空图像并可查看读回。交互重建与性能还需额外证据；窗口 renderer 当前保留串行等待，双槽计算实验不会被描述为完整多帧呈现引擎。

## 实际画面

![dx22_gltf_scene](../../../docs/previews/dx22_gltf_scene.png)
