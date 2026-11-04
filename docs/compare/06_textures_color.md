# 06 · 纹理上传、采样、Mip 与颜色空间

[对照路线](../COMPARE.md) · [交互式对照](index.html#06_textures_color) · [上一章](05_binding_constants.md) · [下一章](07_targets_depth_msaa.md)

“加载了一张图片”至少包含解码、分配、上传、同步、创建访问方式和绑定六个步骤。纹理内容正确但显示不对时，要先判断问题发生在哪一步。

## 上传链路逐项对应

| 步骤 | OpenGL | Vulkan | DX12 |
| --- | --- | --- | --- |
| CPU 像素 | 生成 checker / stb 解码 | 共享 checker RGBA | 共享 checker RGBA |
| GPU 图像 | TexImage2D 内部分配/上传 | Image + device memory | DEFAULT texture resource |
| 中转 | 驱动安排 | HOST_VISIBLE staging buffer | UPLOAD buffer + footprint |
| 使用切换 | 普通命令顺序由驱动处理 | TRANSFER_DST → SHADER_READ + 依赖 | COPY_DEST → PIXEL_SHADER_RESOURCE |
| 访问描述 | texture object/unit | image view + sampler + descriptor | SRV + sampler |

原生 Vulkan 的 texture 是 optimal tiling，不能把 CPU 数组地址当作可采样图像。先把数据写进 staging buffer，录制 buffer-to-image copy，再建立 copy-write 到 fragment-sampled-read 的依赖。临时 staging 在 GPU 完成 copy 后才可释放。

DX12 的纹理拷贝使用 `GetCopyableFootprints`。CPU 图像一行可能是 256 字节，而 API 返回的 RowPitch 还带有对齐；应逐行写入目标 footprint。不能用 `width * height * bytesPerPixel` 一次 memcpy 假装所有 texture upload 都紧密排列。读回纹理也要反过来按 RowPitch 去除填充。

## Sampler 与图像的分工

图像保存 texel，view 选择格式、层和 mip 范围，sampler 决定过滤、寻址和 LOD 行为。Vulkan 的组合采样 descriptor 把图像与 sampler 一起引用，但它们仍是不同对象。DX12 当前用 static sampler；如果要每材质动态改变 sampler，就需要重新设计其绑定方式。

把寻址从 repeat 改为 clamp，改变的是越界 UV 如何访问；把过滤从 nearest 改为 linear，改变的是相邻 texel 如何插值。两者都不会修复上传错行、错误 view format 或 UV 原点不一致。

## 颜色与数据不能一起做 gamma

albedo 通常按 sRGB 编码保存，应在采样时解码到线性空间参与光照。法线、粗糙度、金属度和 AO 是数据，不能因为它们也是 PNG 就使用 sRGB 格式。最终线性 HDR 颜色经过 tone mapping，再进行一次显示编码；对中间结果提前编码会使后续加法和模糊失真。

OpenGL `ImageTexture` 的 srgb 参数用于区分用途；Vulkan/DX12 棋盘采用 sRGB 格式并在输出阶段手动编码。当前演示是教学颜色链路，并不代表已实现显示器 HDR 输出、颜色管理或所有 swapchain 色彩空间。

## Mip 在哪里实现了，哪里没有

OpenGL 纹理调用 glGenerateMipmap 并使用相应 min filter。当前 Vulkan/DX12 示例只创建单级纹理。为它们增加 mip 时，需要同时修改资源级数、上传或生成流程、每级 view/layout/state，以及 sampler 最大 LOD；只打开线性过滤不会凭空生成 mip。

迁移方案：先在 CPU 为 checker 生成一条 mip 链，逐级上传并验证颜色，再考虑 GPU blit/compute 生成。Vulkan 需查询格式是否支持所选过滤与 blit；DX12 的 compute 生成需要逐级 SRV/UAV 和相应依赖。这是扩展练习，不是现有示例的隐藏功能。

## 实验与检查

把 checker 改成四象限纯色，确认方向；再缩小远处物体，比较无 mip 与有 mip 的闪烁。记录纹理尺寸、row pitch、格式、mip 数和 sampler 设定。答案：方向错误先查 UV/行序，整体明暗错误先查颜色空间，远处闪烁再查缩小过滤与 mip，不要用同一种修改处理三类问题。

<!-- generated-source-comparison -->

## 与本仓库实现并排核对

以下切片直接取自当前仓库，省略了前后文。行号用于定位，不能把片段当作完整可编译程序。

| 路线 | 可运行参照 / 范围 |
| --- | --- |
| OpenGL | [05_texture](../../OpenGL/lessons/05_texture/README.md) |
| Vulkan 原生 | [vk06_texture_descriptor](../../Vulkan/native/lessons/vk06_texture_descriptor/README.md) |
| Vulkan / nvpro_core2 | [nv04_vma_staging_texture](../../Vulkan/nvpro/lessons/nv04_vma_staging_texture/README.md) |
| DirectX 12 | [dx06_texture_srv](../../DirectX12/lessons/dx06_texture_srv/README.md) |

### OpenGL · 已实现

sRGB 用于颜色，normal/roughness 等数据应保持线性。

[OpenGL/common/advanced.hpp](../../OpenGL/common/advanced.hpp#L35)，第 35—54 行：

```cpp
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
```

### Vulkan 原生 · 已实现

staging 到 optimal image，先传输后采样并建立依赖。

[Vulkan/common/scene_vk.hpp](../../Vulkan/common/scene_vk.hpp#L224)，第 224—267 行：

```cpp
        auto data = checker();
        VkBufferResource upload;
        upload.init(c, data.size(), VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
        upload.upload(data.data(), data.size());
        texture.init(c,
                     {64, 64},
                     VK_FORMAT_R8G8B8A8_SRGB,
                     VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                     VK_IMAGE_ASPECT_COLOR_BIT);

        // 纹理先切到传输目标布局，拷贝完成后再切到 shader 只读布局供片元阶段采样。
        immediate(c, [&](VkCommandBuffer cmd) {
            barrier(cmd,
                    texture.image,
                    VK_IMAGE_LAYOUT_UNDEFINED,
                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    VK_PIPELINE_STAGE_2_NONE,
                    0,
                    VK_PIPELINE_STAGE_2_COPY_BIT,
                    VK_ACCESS_2_TRANSFER_WRITE_BIT);
            VkBufferImageCopy copy{};
            copy.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
            copy.imageExtent = {64, 64, 1};
            vkCmdCopyBufferToImage(
                cmd, upload.buffer, texture.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy);
            barrier(cmd,
                    texture.image,
                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    VK_PIPELINE_STAGE_2_COPY_BIT,
                    VK_ACCESS_2_TRANSFER_WRITE_BIT,
                    VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
                    VK_ACCESS_2_SHADER_SAMPLED_READ_BIT);
        });
        VkSamplerCreateInfo si{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
        si.magFilter = si.minFilter = VK_FILTER_LINEAR;
        si.addressModeU = si.addressModeV = si.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        si.maxLod = 0;
        vkCheck(vkCreateSampler(c.device, &si, nullptr, &sampler));

        // binding 0 对应每帧 UBO，binding 1 对应纹理和采样器；编号与 GLSL 保持一致。
        VkDescriptorSetLayoutBinding bindings[2] = {
            {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr},
            {1,
```

### Vulkan / nvpro_core2 · 已实现

框架层负责分配目标和顶点上传；纹理细节沿 SceneVk 展开。

[Vulkan/nvpro/common/nvapp_course.hpp](../../Vulkan/nvpro/common/nvapp_course.hpp#L61)，第 61—94 行：

```cpp
    void onAttach(nvapp::Application* application) override {
        app = application;
        auto queue = app->getQueue(0);
        c = {app->getInstance(),
             app->getPhysicalDevice(),
             app->getDevice(),
             queue.queue,
             queue.familyIndex};

        // VMA 管理分配与绑定，调用方仍需声明用途并保证 GPU 使用期间资源存活。
        VmaAllocatorCreateInfo ai{};
        ai.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
        ai.physicalDevice = c.physical;
        ai.device = c.device;
        ai.instance = c.instance;
        ai.vulkanApiVersion = VK_API_VERSION_1_4;
        vkCheck(allocator.init(ai));
        nvvk::RenderTarget::CreateInfo info{};
        info.alloc = &allocator;
        info.colorFormats = {VK_FORMAT_R8G8B8A8_UNORM};
        info.depthFormat = VK_FORMAT_D32_SFLOAT;
        info.colorUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT |
                          VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        info.debugName = "Course viewport";
        vkCheck(target.init(info));
        if (mode >= 3)
            scene = std::make_unique<SceneVk>(c, mode, VK_FORMAT_R8G8B8A8_UNORM, nvShaders);
        if (mode == 6) {
            auto& vertices = scene->mesh.vertices;
            vkCheck(allocator.createBuffer(uploadedVertices,
                                           vertices.size() * sizeof(Vertex),
                                           VK_BUFFER_USAGE_2_VERTEX_BUFFER_BIT));

            // 先把数据加入 staging，再录制拷贝并等待 GPU；完成后才能释放临时上传器。
```

### DirectX 12 · 已实现

按 RowPitch 逐行填 upload；创建 SRV 不会复制像素。

[DirectX12/common/dx12_course.hpp](../../DirectX12/common/dx12_course.hpp#L284)，第 284—323 行：

```cpp
        D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint;
        UINT rows;
        UINT64 rowBytes, total;

        // 纹理上传不能假定行数据紧密排列；按 footprint 的对齐行距逐行复制。
        c.device->GetCopyableFootprints(&image, 0, 1, 0, &footprint, &rows, &rowBytes, &total);
        auto upload = c.buffer(total, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ);
        void* mapped;
        D3D12_RANGE empty{0, 0};
        dxCheck(upload->Map(0, &empty, &mapped));
        auto data = checker();
        for (UINT y = 0; y < rows; ++y)
            std::memcpy(static_cast<uint8_t*>(mapped) + footprint.Offset +
                            size_t(y) * footprint.Footprint.RowPitch,
                        data.data() + size_t(y) * 64 * 4,
                        64 * 4);
        upload->Unmap(0, nullptr);
        c.begin();
        D3D12_TEXTURE_COPY_LOCATION source{}, destination{};
        source.pResource = upload.Get();
        source.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        source.PlacedFootprint = footprint;
        destination.pResource = texture.Get();
        destination.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        c.cmd->CopyTextureRegion(&destination, 0, 0, 0, &source, nullptr);
        auto b = transition(texture.Get(),
                            D3D12_RESOURCE_STATE_COPY_DEST,
                            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        c.cmd->ResourceBarrier(1, &b);
        c.submit();
        D3D12_DESCRIPTOR_HEAP_DESC heap{};
        heap.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        heap.NumDescriptors = 1;
        heap.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        dxCheck(c.device->CreateDescriptorHeap(&heap, IID_PPV_ARGS(&srvHeap)));
        D3D12_SHADER_RESOURCE_VIEW_DESC srv{};
        srv.Format = image.Format;
        srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srv.Texture2D.MipLevels = 1;
```
