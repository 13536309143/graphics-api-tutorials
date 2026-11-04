# 14 · 从黑屏到性能：证据驱动的调试

[对照路线](../COMPARE.md) · [交互式对照](index.html#14_debug_performance) · [上一章](13_multipass_effects.md) · [下一章](15_framework_architecture.md)

本章把 CPU 断点、API 验证、GPU 抓帧和性能分析分开。它们观察不同层次，不能互相替代。

## 黑屏排查的固定顺序

先确认进程和循环存活，再确认 shader 编译/管线创建成功，然后看 draw 是否提交、顶点输出是否合理、目标是否正确、深度/模板是否拒绝、最终是否呈现。跳过前面直接改灯光，常会把无关问题越改越复杂。

| 证据 | 适合回答的问题 | 不能单独证明什么 |
| --- | --- | --- |
| CPU 断点/变量 | 哪条路径执行、创建参数是什么 | GPU 已执行完或像素正确 |
| validation / debug layer | 哪条使用约束被破坏 | 公式符合视觉预期 |
| mesh/pipeline viewer | GPU 当时实际读取与绑定什么 | 所有帧都无竞态 |
| 图像读回 | 固定输入是否得到有效输出 | 带宽、性能或跨设备都正确 |
| GPU timestamp/工具分析 | 某段 GPU 工作耗时 | 整个应用的端到端延迟 |

OpenGL 当前用 shader 日志和帧尾 glGetError；没有假称已经加上完整 KHR_debug 系统。Vulkan 的 VUID 是查具体有效用法的入口；DX12 InfoQueue 保留严重度与描述。先处理第一条根因错误，后面的报错常是错误对象继续传播的结果。

## 按资源追踪，而非只按函数追踪

选择一张纹理，从 CPU 字节、upload、GPU image、view、descriptor、draw 事件到目标像素一路记录。若最终纹理是空的，往回检查 copy 事件；若像素已正确而屏幕错误，检查最终输出或呈现。nvpro 抓帧要找场景离屏 draw，不能只查看最后的 ImGui 合成。

使用 RenderDoc 检查 OpenGL/Vulkan 的事件、顶点和资源；DX12 可用 PIX 看 root signature、PSO、resource state 和 GPU 工作。每次工具安装版本的支持范围以其官方文档为准，本教程不把某个高级 shader 调试功能视为所有设备都可用。

## 为什么现在不能拿 FPS 排 API 名次

当前三条路线的场景、帧等待、UI 和 shader 编译阶段并不完全相同；Vulkan/DX12 还按帧串行等待。窗口垂直同步会把快慢都限制到相同刷新率。用 Debug、不同分辨率、不同三角形数跑一次 FPS，再得出某 API 更快，是混合了多个变量的结论。

公平实验至少固定工作量、分辨率、时间、贴图、采样数、shader 逻辑和呈现模式，使用 Release，预热并排除编译/首次分配，分别报告 CPU 提交时间、GPU pass 时间和整体帧时间。采集多次，记录中位数和波动，不以一次最小值作结论。

GPU timestamp 通常需要延迟读取，避免为了读取本帧结果又强制等待，从而改变被测系统。离屏不呈现测试与真实窗口帧循环衡量的也不是同一个问题。当前仓库没有完整统一 benchmark，下面的性能记录是实验方案。

## 实验：建立一份错误报告

固定运行一个三角形课，记录提交版本、设备、驱动、配置、预期/实际结果和最小复现步骤。出现错误时附第一条 validation/InfoQueue 信息以及相关 draw 事件。修改后证明错误消失且原本正确输出保留，不要只写“重装后好了”。

自测：CPU 的 draw 调用只用了几微秒，能说明 GPU 也很快吗？不能；调用时间可能只覆盖录制/提交。需要 GPU 时间证据。反过来，CPU 在 fence 等待很久也可能是在等之前的工作，并不必然代表该行本身计算复杂。

具体工具步骤见[完整调试指南](../DEBUGGING.md)，验证概念见 [Khronos validation overview](https://docs.vulkan.org/guide/latest/validation_overview.html)。

<!-- generated-source-comparison -->

## 与本仓库实现并排核对

以下切片直接取自当前仓库，省略了前后文。行号用于定位，不能把片段当作完整可编译程序。

| 路线 | 可运行参照 / 范围 |
| --- | --- |
| OpenGL | [02_triangle](../../OpenGL/lessons/02_triangle/README.md) |
| Vulkan 原生 | [vk03_triangle](../../Vulkan/native/lessons/vk03_triangle/README.md) |
| Vulkan / nvpro_core2 | [nv03_triangle](../../Vulkan/nvpro/lessons/nv03_triangle/README.md) |
| DirectX 12 | [dx03_triangle](../../DirectX12/lessons/dx03_triangle/README.md) |

### OpenGL · 已实现

帧尾错误号辅助定位；完整问题还需要查看绑定和实际 draw。

[OpenGL/common/app.hpp](../../OpenGL/common/app.hpp#L123)，第 123—130 行：

```cpp
        GLenum e = glGetError();
        if (e != GL_NO_ERROR)
            throw std::runtime_error("OpenGL error: " + std::to_string(e));
        glfwSwapBuffers(window);
        ++frames;
    }
    void clear() {
        glClearColor(0.035f, 0.055f, 0.09f, 1);
```

### Vulkan 原生 · 已实现

验证错误与外部 loader 诊断分开，先处理首个根因。

[Vulkan/native/common/native_app.hpp](../../Vulkan/native/common/native_app.hpp#L9)，第 9—26 行：

```cpp
inline VKAPI_ATTR VkBool32 VKAPI_CALL validation(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
                                                 VkDebugUtilsMessageTypeFlagsEXT type,
                                                 const VkDebugUtilsMessengerCallbackDataEXT* data,
                                                 void*) {
    if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
        if (type & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT) {
            ++validationErrors;
            std::cerr << "VALIDATION ERROR: ";
        } else
            std::cerr << "VULKAN LOADER DIAGNOSTIC: ";
        std::cerr << data->pMessage << '\n';
    }
    return VK_FALSE;
}

// 手动创建 Instance → PhysicalDevice/queue family → Device/Queue；销毁时反向处理。
struct NativeDevice {
    VkContextRef c;
```

### Vulkan / nvpro_core2 · 已实现

无窗口模式读回真实视口目标，不能代替长期交互和性能验证。

[Vulkan/nvpro/common/nvapp_course.hpp](../../Vulkan/nvpro/common/nvapp_course.hpp#L174)，第 174—203 行：

```cpp
    void onLastHeadlessFrame() override {
        auto size = target.getSize();
        VkBufferResource readback;
        readback.init(
            c, VkDeviceSize(size.width) * size.height * 4, VK_BUFFER_USAGE_TRANSFER_DST_BIT);
        immediate(c, [&](VkCommandBuffer cmd) {
            barrier(cmd,
                    target.getColorImage(),
                    VK_IMAGE_LAYOUT_GENERAL,
                    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                    VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
                    VK_ACCESS_2_MEMORY_WRITE_BIT,
                    VK_PIPELINE_STAGE_2_COPY_BIT,
                    VK_ACCESS_2_TRANSFER_READ_BIT);
            VkBufferImageCopy copy{};
            copy.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
            copy.imageExtent = {size.width, size.height, 1};
            vkCmdCopyImageToBuffer(cmd,
                                   target.getColorImage(),
                                   VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                   readback.buffer,
                                   1,
                                   &copy);
            VkMemoryBarrier2 host{VK_STRUCTURE_TYPE_MEMORY_BARRIER_2};
            host.srcStageMask = VK_PIPELINE_STAGE_2_COPY_BIT;
            host.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
            host.dstStageMask = VK_PIPELINE_STAGE_2_HOST_BIT;
            host.dstAccessMask = VK_ACCESS_2_HOST_READ_BIT;
            VkDependencyInfo dependency{VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
            dependency.memoryBarrierCount = 1;
```

### DirectX 12 · 已实现

ERROR/CORRUPTION 使 smoke 失败；抓帧再看数据和管线。

[DirectX12/common/dx12_course.hpp](../../DirectX12/common/dx12_course.hpp#L140)，第 140—165 行：

```cpp
    void checkMessages() {
        if (!messages)
            return;
        UINT64 count = messages->GetNumStoredMessagesAllowedByRetrievalFilter();
        for (UINT64 i = 0; i < count; ++i) {
            SIZE_T bytes = 0;
            messages->GetMessage(i, nullptr, &bytes);
            std::vector<uint8_t> storage(bytes);
            auto m = reinterpret_cast<D3D12_MESSAGE*>(storage.data());
            dxCheck(messages->GetMessage(i, m, &bytes));
            if (m->Severity == D3D12_MESSAGE_SEVERITY_ERROR ||
                m->Severity == D3D12_MESSAGE_SEVERITY_CORRUPTION)
                throw std::runtime_error(std::string("D3D12 validation: ") + m->pDescription);
        }
        messages->ClearStoredMessages();
    }
    ComPtr<ID3D12Resource> buffer(UINT64 size,
                                  D3D12_HEAP_TYPE heap,
                                  D3D12_RESOURCE_STATES state,
                                  D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE) {
        D3D12_HEAP_PROPERTIES hp{};
        hp.Type = heap;
        D3D12_RESOURCE_DESC rd{};
        rd.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        rd.Width = size;
        rd.Height = 1;
```
