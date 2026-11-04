# 第 22 课 · 窗口变化与交换链重建 · Vulkan 原生

[54 课目录](../../COURSE.md) · [对照精讲](../../../../docs/compare/07_targets_depth_msaa.md) · [上一课](../21_frames_in_flight/README.md) · [下一课](../23_vertex_input/README.md)

## 本课目标与先修

学习目标：解释“窗口变化与交换链重建”解决的问题，找到实际对象和参数，运行对应实验并用数据/画面证明。先完成前一课，并能说明它交付的对象与完成条件；本课在该依赖上增加新契约。

## 为什么需要这一层

重建是完整生命周期事件，不能只修改两个宽高变量。先保证旧资源不再被使用，再释放依赖旧图像的 view/附件，重新查询配置，更新投影和动态矩形。程序最小化时应等待有效尺寸。

## Vulkan 原生 的对象、参数与执行推导

处理 OUT_OF_DATE/SUBOPTIMAL 以及 framebuffer 尺寸变化。cleanup 等 GPU 后释放 scene/depth/MSAA/view/semaphore，重新创建，并重新执行实验的资源配置回调。格式变化可能影响 pipeline，不能永远假定只有 extent 改变。

## 对照 DirectX 12：相同问题的另一种分工

ResizeBuffers 前释放所有 back buffer 引用并等待相关 GPU 工作；随后重建 RTV/DSV、depth 与 MSAA/离屏目标。只释放一个局部 ComPtr 而仍有其他引用，不满足重建要求。

## 关键参数与可检查不变量

| 检查项 | Vulkan 表达 | DX12 表达 | 不变量 / 证据 |
| --- | --- | --- | --- |
| 重建前置 | GPU 不再用旧目标 | GPU 完成且旧 backbuffer 引用释放 | 先断依赖，再分配新目标 |
| 恢复参数 | extent / views / projection | size / RTV/DSV / projection | viewport 与 aspect 也要更新 |
| 错误分支 | OUT_OF_DATE / SUBOPTIMAL | ResizeBuffers/HRESULT | 可恢复失败不留下不可完成等待 |

先为每项指出源码中的创建/绑定位置，以及证明它成立的实际日志、字节或图像。数值中的设备限制以查询为准；固定实验尺寸不当作所有设备的上限。

## nvpro_core2 框架对照

onResize 与 RenderTarget::update 提供明确接入点。target 变化后更新 ImTexture；若使用自己的缓存图像，也要给它同样的重建契约。

这部分是原生 Vulkan 各课的框架对照，框架可运行入口仍为原有六课。如果本节需要新接入工作，会按正文说明；不把原生实验当作已完成的框架实现。

## 运行与实际检查

| 路线 | 实际实验 |
| --- | --- |
| Vulkan 原生 | [vk02_swapchain](../../lessons/vk02_swapchain/README.md) |
| Vulkan 原生 | [vk40_surface_support](../../lessons/vk40_surface_support/README.md) |
| DirectX 12 | [dx02_swapchain](../../../../DirectX12/lessons/dx02_swapchain/README.md) |
| DirectX 12 | [dx40_surface_support](../../../../DirectX12/lessons/dx40_surface_support/README.md) |

从合集根目录执行；普通图形程序 Esc 退出，设备/计算/诊断程序输出结果后结束。从“开始学习.cmd”进入本页时，表格可直接点击运行。

```powershell
cmake --preset vulkan-native
cmake --build --preset vulkan-native-debug --target vk02_swapchain --parallel 4
.\Vulkan\native\build\bin\Debug\vk02_swapchain.exe
ctest --preset vulkan-native-debug -R "^vk02_swapchain$" --output-on-failure
```

检查三个层次：创建/绑定/提交的接口约束；数据读回或目标画面；资源使用结束后的回收。诊断实验用合法信息或查询证明渠道可用，不故意保留 API 错误。图形 smoke 的像素检查与数值实验的逐项检查不同，具体实现见源码。

## 关键实现：从当前源码定位

来自 [Vulkan/native/common/native_app.hpp](../../common/native_app.hpp#L207)，第 207—250 行。下列是当前仓库的局部切片，可能在函数中间结束；完整上下文请打开文件，切片不作为单独可编译程序。

```cpp
    void recreate() {
        cleanup();
        VkSurfaceCapabilitiesKHR caps;
        vkCheck(
            vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device.c.physical, device.surface, &caps));
        uint32_t count;
        vkCheck(vkGetPhysicalDeviceSurfaceFormatsKHR(
            device.c.physical, device.surface, &count, nullptr));
        std::vector<VkSurfaceFormatKHR> formats(count);
        vkCheck(vkGetPhysicalDeviceSurfaceFormatsKHR(
            device.c.physical, device.surface, &count, formats.data()));
        auto selected = formats[0];
        for (auto f : formats)
            if (f.format == VK_FORMAT_B8G8R8A8_UNORM &&
                f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
                selected = f;
        format = selected.format;
        extent = caps.currentExtent;
        if (extent.width == UINT32_MAX) {
            int w, h;
            glfwGetFramebufferSize(window, &w, &h);
            extent = {
                std::clamp(uint32_t(w), caps.minImageExtent.width, caps.maxImageExtent.width),
                std::clamp(uint32_t(h), caps.minImageExtent.height, caps.maxImageExtent.height)};
        }
        VkImageUsageFlags usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                                  VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        if ((caps.supportedUsageFlags & usage) != usage)
            throw std::runtime_error("Surface does not support transfer readback/blit");
        VkSwapchainCreateInfoKHR si{VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
        si.surface = device.surface;
        si.minImageCount = caps.minImageCount + 1;
        if (caps.maxImageCount)
            si.minImageCount = std::min(si.minImageCount, caps.maxImageCount);
        si.imageFormat = format;
        si.imageColorSpace = selected.colorSpace;
        si.imageExtent = extent;
        si.imageArrayLayers = 1;
        si.imageUsage = usage;
        si.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        si.preTransform = caps.currentTransform;
        si.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        si.presentMode = VK_PRESENT_MODE_FIFO_KHR;
        si.clipped = VK_TRUE;
```

阅读时按“前置对象 → 参数/布局 → 录制/调用 → 完成/可见性 → 释放”五步标注。共享 helper 减少重复代码，但不会取消每课的 API 契约。

## 受控修改实验

普通交互模式下反复拖动、最小化、恢复窗口，记录会触发哪些重建路径。

先记录预测，再只改一个因素；保留基线与修改后的输出。不要同时改变 shader、资源范围和同步，这样即使结果变好也无法归因。

### 参考推导与通过标准

应避免零尺寸分配/除零和旧图像引用。已有 smoke 检查有限帧输出，反复 resize 属于额外手动验收，不能混写成已自动覆盖所有组合。

## 常见失败与排查顺序

旧 descriptor/image view 仍指向被释放附件，或新尺寸深度与颜色不同，都是比投影参数更早的问题。

先看第一条错误和当前执行路径，再看实际数据/绑定，最后看 shader 与画面。需要 GPU 证据时抓取正常交互帧；仅四帧的 smoke 适合自动检查，不适合手动抓帧。

## 自测与本课交付

能否不看源码回答：普通交互模式下反复拖动、最小化、恢复窗口，记录会触发哪些重建路径。

提交一份简短记录：输入与假设、唯一修改、预期、实际证据、原因、是否恢复基线。如果只能复述函数名而说不清何时允许访问/释放，先回到本课的数据流。

## 官方阅读位置与当前工程差异

[指定 Vulkan 教程对应位置](https://docs.vulkan.net.cn/tutorial/latest/03_Drawing_a_triangle/04_Swap_chain_recreation.html)。DX12 路线按同一概念顺序建立映射，其对象与约束以 Microsoft 文档和本后端代码为准。49—54 课包含在基础/compute 之后加入的工程扩展。

[版本与资料说明](../../../../docs/REFERENCES.md) · [调试](../../../../docs/DEBUGGING.md) · [功能完成度](../../../../docs/FEATURE_MATRIX.md)

## 固定输入的实际 GPU 读回

![vk02_swapchain](../../../../docs/previews/vk02_swapchain.png)
