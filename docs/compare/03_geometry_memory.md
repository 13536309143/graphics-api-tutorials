# 03 · 顶点、索引与 GPU 内存

[对照路线](../COMPARE.md) · [交互式对照](index.html#03_geometry_memory) · [上一章](02_triangle_pipeline.md) · [下一章](04_transforms_camera.md)

本章把“一个数组”拆成四层：CPU 数据、GPU 资源、底层内存、解释布局的接口。顶点看起来错乱时，四层中的任一层出错都可能得到相似画面。

## 先核对字节，再讨论形状

共享 `Vertex` 是 position 三个 float、normal 三个 float、UV 两个 float，共 32 字节。对应偏移为 0、12、24。若误把 stride 写成 24，第二个顶点就会从前一个顶点的 UV 处开始读取；如果索引类型用 uint16 而内存实际是 uint32，三角形连接也会完全错误。

OpenGL 基础 `Mesh` 使用八个连续 float，基本布局同样是 32 字节。导入模型的 `ImportedMesh::Vertex` 还包含 joints/weights，不能继续套用 32 字节；它使用 `offsetof` 描述成员，骨骼索引走整数 attribute 接口。

| 责任 | OpenGL | Vulkan | DX12 |
| --- | --- | --- | --- |
| 储存顶点 | buffer object | VkBuffer + 绑定内存 | ID3D12Resource |
| 描述布局 | VAO 中的 attribute 配置 | binding/attribute descriptions | input layout + VBV |
| 索引解释 | DrawElements 的 type | BindIndexBuffer 的 index type | IBV 的 format |
| 内存选择 | 由驱动管理细节 | memoryTypeBits 与属性筛选 | DEFAULT/UPLOAD/READBACK heap |

## 本仓库如何上传

OpenGL 用 `glBufferData` 提交顶点；驱动可以在内部复制和调度。原生 Vulkan 的 `VkBufferResource` 默认申请 HOST_VISIBLE 和 HOST_COHERENT 内存，map 后 memcpy。这让初学者容易追踪数据，但不应被描述为独立显卡静态几何的最佳传输策略。DX12 的顶点/索引同样直接保存在 UPLOAD heap，教学目标是观察资源和输入视图。

nv04 给出更接近静态几何的路径：VMA 创建 device-local 顶点缓冲，StagingUploader 临时保存 CPU 数据，录制 copy，等待完成，再释放上传器。纹理也有 CPU 可见缓冲到 optimal image 的拷贝，但它在公共 SceneVk 中实现，不能把两条路径混称为同一个 VMA 纹理上传例子。

“device local”与“host visible”不是逻辑上的互斥概念，统一内存设备可能同时具备。程序应该查询属性和使用场景，不应从显卡品牌猜某个 memory type 索引。非 coherent 内存还涉及 flush/invalidate 及非一致原子尺寸对齐；本代码选 coherent 并没有消除 CPU/GPU 的执行同步要求。

## 索引复用的边界

一个立方体不总是八个顶点。同一空间位置在不同面可能具有不同法线或 UV，GPU 顶点索引指向的是一整套属性，因此硬边立方体通常需要每面独立顶点。共享场景建立 24 个顶点和 36 个索引；它与“八个几何角点”并不矛盾。

## 实验：从抓帧验证布局

打开 vk04/dx04 或 OpenGL 索引课，在 mesh viewer 中查看前三个 position 和 UV，核对 CPU 数组值。然后只改变一个顶点的 X，确认只有相关三角形移动。完成后恢复基线，再比较索引版和非索引版的顶点数量、索引数量与 draw 参数。

思考：减少顶点数是否一定更快？参考答案：不一定；索引局部性、缓存命中、顶点阶段成本和带宽共同影响结果。正确性课只验证数据含义，性能结论需要额外测量。

迁移练习：为原生 Vulkan 顶点增加 staging 路径。写出目标 usage 必须包含的 TRANSFER_DST 和 VERTEX_BUFFER，以及 copy-write 到 vertex-attribute-read 的依赖。该练习是设计任务，当前 vk04 默认仍使用直接可见缓冲。[官方 staging 章节](https://docs.vulkan.org/tutorial/latest/04_Vertex_buffers/02_Staging_buffer.html) 可用于核对两级上传思路。

<!-- generated-source-comparison -->

## 与本仓库实现并排核对

以下切片直接取自当前仓库，省略了前后文。行号用于定位，不能把片段当作完整可编译程序。

| 路线 | 可运行参照 / 范围 |
| --- | --- |
| OpenGL | [03_indexed](../../OpenGL/lessons/03_indexed/README.md) |
| Vulkan 原生 | [vk04_vertex_buffer](../../Vulkan/native/lessons/vk04_vertex_buffer/README.md) |
| Vulkan / nvpro_core2 | [nv04_vma_staging_texture](../../Vulkan/nvpro/lessons/nv04_vma_staging_texture/README.md) |
| DirectX 12 | [dx04_vertex_buffer](../../DirectX12/lessons/dx04_vertex_buffer/README.md) |

### OpenGL · 已实现

EBO 绑定属于 VAO 状态，attribute stride 与 CPU 数据一致。

[OpenGL/common/app.hpp](../../OpenGL/common/app.hpp#L217)，第 217—264 行：

```cpp
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
```

### Vulkan 原生 · 已实现

先满足 memoryTypeBits 与属性，再绑定资源；coherent 不等于执行同步。

[Vulkan/common/scene_vk.hpp](../../Vulkan/common/scene_vk.hpp#L29)，第 29—73 行：

```cpp
struct VkBufferResource {
    VkDevice device{};
    VkBuffer buffer{};
    VkDeviceMemory memory{};
    VkDeviceSize size{};
    VkBufferResource() = default;
    VkBufferResource(const VkBufferResource&) = delete;
    VkBufferResource& operator=(const VkBufferResource&) = delete;
    void init(const VkContextRef& c,
              VkDeviceSize bytes,
              VkBufferUsageFlags usage,
              VkMemoryPropertyFlags flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) {
        device = c.device;
        size = bytes;
        VkBufferCreateInfo info{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
        info.size = bytes;
        info.usage = usage;
        info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        vkCheck(vkCreateBuffer(device, &info, nullptr, &buffer));
        VkMemoryRequirements req;
        vkGetBufferMemoryRequirements(device, buffer, &req);
        VkMemoryAllocateInfo alloc{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
        alloc.allocationSize = req.size;
        alloc.memoryTypeIndex = memoryType(c, req.memoryTypeBits, flags);
        vkCheck(vkAllocateMemory(device, &alloc, nullptr, &memory));
        vkCheck(vkBindBufferMemory(device, buffer, memory, 0));
    }

    // 此处缓冲使用 HOST_VISIBLE | HOST_COHERENT；非 coherent 内存还需显式 flush。
    void upload(const void* data, size_t bytes) {
        void* p;
        vkCheck(vkMapMemory(device, memory, 0, size, 0, &p));
        std::memcpy(p, data, bytes);
        vkUnmapMemory(device, memory);
    }
    ~VkBufferResource() {
        if (buffer)
            vkDestroyBuffer(device, buffer, nullptr);
        if (memory)
            vkFreeMemory(device, memory, nullptr);
    }
};
struct VkImageResource {
    VkDevice device{};
```

### Vulkan / nvpro_core2 · 已实现

本段上传 device-local 顶点；纹理路径在 SceneVk，不能混作同一 helper。

[Vulkan/nvpro/common/nvapp_course.hpp](../../Vulkan/nvpro/common/nvapp_course.hpp#L88)，第 88—107 行：

```cpp
        if (mode == 6) {
            auto& vertices = scene->mesh.vertices;
            vkCheck(allocator.createBuffer(uploadedVertices,
                                           vertices.size() * sizeof(Vertex),
                                           VK_BUFFER_USAGE_2_VERTEX_BUFFER_BIT));

            // 先把数据加入 staging，再录制拷贝并等待 GPU；完成后才能释放临时上传器。
            nvvk::StagingUploader upload;
            upload.init(&allocator);
            vkCheck(upload.appendBuffer(uploadedVertices, 0, std::span<const Vertex>(vertices)));
            auto cmd = app->createTempCmdBuffer();
            upload.cmdUploadAppended(cmd);
            app->submitAndWaitTempCmdBuffer(cmd);
            upload.deinit();
            scene->overrideVertices = uploadedVertices.buffer;
            std::cout << "VMA device-local vertex upload complete\n";
        }
    }

    // 视口大小变化时重建 RenderTarget，并把新的 image view 交给 ImGui 显示。
```

### DirectX 12 · 已实现

UPLOAD 顶点和 index buffer view 保持清楚，性能优化可再改 DEFAULT。

[DirectX12/common/dx12_course.hpp](../../DirectX12/common/dx12_course.hpp#L246)，第 246—283 行：

```cpp
    DxScene(DxDevice& context, int lesson) : c(context), mode(lesson), mesh(geometry(mode)) {
        if (mode >= 4) {
            UINT size = UINT(mesh.vertices.size() * sizeof(Vertex));
            vertices = c.buffer(size, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ);
            void* mapped;
            D3D12_RANGE empty{0, 0};
            dxCheck(vertices->Map(0, &empty, &mapped));
            std::memcpy(mapped, mesh.vertices.data(), size);
            vertices->Unmap(0, nullptr);
            vb = {vertices->GetGPUVirtualAddress(), size, sizeof(Vertex)};
            if (!mesh.indices.empty()) {
                UINT bytes = UINT(mesh.indices.size() * 4);
                indices =
                    c.buffer(bytes, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ);
                dxCheck(indices->Map(0, &empty, &mapped));
                std::memcpy(mapped, mesh.indices.data(), bytes);
                indices->Unmap(0, nullptr);
                ib = {indices->GetGPUVirtualAddress(), bytes, DXGI_FORMAT_R32_UINT};
            }
        }

        // CBV GPU 地址要求 256 字节对齐，因此给 64 字节的帧矩阵预留 256 字节。
        uniform = c.buffer(256, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ);
        D3D12_HEAP_PROPERTIES hp{};
        hp.Type = D3D12_HEAP_TYPE_DEFAULT;
        D3D12_RESOURCE_DESC image{};
        image.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        image.Width = image.Height = 64;
        image.DepthOrArraySize = 1;
        image.MipLevels = 1;
        image.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        image.SampleDesc.Count = 1;
        dxCheck(c.device->CreateCommittedResource(&hp,
                                                  D3D12_HEAP_FLAG_NONE,
                                                  &image,
                                                  D3D12_RESOURCE_STATE_COPY_DEST,
                                                  nullptr,
                                                  IID_PPV_ARGS(&texture)));
```
