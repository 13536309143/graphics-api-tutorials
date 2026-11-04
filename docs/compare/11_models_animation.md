# 11 · 模型、材质、节点与骨骼：共享什么，重写什么

[对照路线](../COMPARE.md) · [交互式对照](index.html#11_models_animation) · [上一章](10_compute_readback.md) · [下一章](12_lighting_pbr_ibl.md)

模型加载不是某个图形 API 的专用命令。文件解析得到 CPU 场景，后端再把网格、图片和材质参数转换成 GPU 资源。将这两层分开，才有可能把 OpenGL 的模型功能迁到 Vulkan/DX12。

## 当前真实实现

OpenGL 14/15 通过 Assimp 读取 OBJ/glTF，31 演示骨骼动画，34 将模型与材质/环境组合。Vulkan 和 DX12 当前只使用程序生成的四边形、立方体和球体；没有完整 glTF 导入与蒙皮后端。下面的迁移步骤是工程设计，不计入已运行课程。

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

第一步只导入单个无贴图静态 mesh，保留 position/normal/UV，使用固定颜色。第二步支持节点 global transform 和多个 mesh。第三步加入纹理与材质，检查缓存键是否区分 sRGB/linear。第四步才扩展 GPU 顶点布局、joint buffer 和每帧骨骼更新。

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
| Vulkan 原生 | 迁移设计 |
| Vulkan / nvpro_core2 | 迁移设计 |
| DirectX 12 | 迁移设计 |

### OpenGL · 已实现

按 mesh 节点变换与 inverse bind pose 构造局部蒙皮矩阵。

[OpenGL/common/model.hpp](../../OpenGL/common/model.hpp#L304)，第 304—332 行：

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

### Vulkan 原生 · 迁移设计

设计：CPU MeshData → VkBuffer；骨骼 UBO/SSBO；节点/材质解析可共享。


### Vulkan / nvpro_core2 · 迁移设计

框架有资产相关辅助，但本课程未接入完整 glTF/skin；需要单独实现并验收。


### DirectX 12 · 迁移设计

设计：扩展 VBV/input layout 加 joints/weights，骨骼经 CBV/SRV 提供。
