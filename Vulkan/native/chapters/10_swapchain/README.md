# 第 10 课 · 交换链选择、格式与呈现模式 · Vulkan 原生

[54 课目录](../../COURSE.md) · [对照精讲](../../../../docs/compare/07_targets_depth_msaa.md) · [上一课](../09_surface/README.md) · [下一课](../11_image_views/README.md)

## 本课目标与先修

学习目标：解释“交换链选择、格式与呈现模式”解决的问题，找到实际对象和参数，运行对应实验并用数据/画面证明。先完成前一课，并能说明它交付的对象与完成条件；本课在该依赖上增加新契约。

## 为什么需要这一层

交换链图像由呈现系统管理，其数量、尺寸和格式不是任意应用纹理的全部自由参数。先查询范围，再选择配置；零值或特殊 extent 往往具有约定含义，要按接口解释。

## Vulkan 原生 的对象、参数与执行推导

检查 min/max image count、currentExtent、supportedUsageFlags、surface formats 与 present modes。maxImageCount=0 表示没有给出上限，不是零张图像。当前例子选择 FIFO，并要求读回/blit 用途；这会比只呈现的最小程序需求更多。

## 对照 DirectX 12：相同问题的另一种分工

课程选择两个 RGBA8 flip-discard back buffer，GetCurrentBackBufferIndex 决定本帧目标。显示同步与 tearing 的支持/flag 要另行检查，不能仅把 Present 参数设为零就称作公平性能测试。

## 关键参数与可检查不变量

| 检查项 | Vulkan 表达 | DX12 表达 | 不变量 / 证据 |
| --- | --- | --- | --- |
| 图像数量 | min/max image count | BufferCount=2 | Vulkan max=0 表示未给上限 |
| 尺寸 | currentExtent / min/max extent | ResizeBuffers 的宽高 | 最小化时不使用零尺寸目标 |
| 本项目格式 | RGBA8/BGRA8 根据 surface | RGBA8 flip-discard | 管线目标格式跟随实际附件 |

先为每项指出源码中的创建/绑定位置，以及证明它成立的实际日志、字节或图像。数值中的设备限制以查询为准；固定实验尺寸不当作所有设备的上限。

## nvpro_core2 框架对照

Application 管理交换链，用户 viewport 是另一组离屏目标。比较尺寸时区分外层窗口与 Viewport 实际像素区域。

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

解释为何两帧 CPU 资源槽与三张交换链图像可以同时存在。

先记录预测，再只改一个因素；保留基线与修改后的输出。不要同时改变 shader、资源范围和同步，这样即使结果变好也无法归因。

### 参考推导与通过标准

前者跟踪应用可变资源的在途使用期，后者由 acquire/呈现系统选择。两个索引各有复用条件，不能靠数值恰好相等统一。

## 常见失败与排查顺序

窗口最小化导致零尺寸时不要创建零大小投影或附件；先等待可用 framebuffer 尺寸再继续。

先看第一条错误和当前执行路径，再看实际数据/绑定，最后看 shader 与画面。需要 GPU 证据时抓取正常交互帧；仅四帧的 smoke 适合自动检查，不适合手动抓帧。

## 自测与本课交付

能否不看源码回答：解释为何两帧 CPU 资源槽与三张交换链图像可以同时存在。

提交一份简短记录：输入与假设、唯一修改、预期、实际证据、原因、是否恢复基线。如果只能复述函数名而说不清何时允许访问/释放，先回到本课的数据流。

## 官方阅读位置与当前工程差异

[指定 Vulkan 教程对应位置](https://docs.vulkan.net.cn/tutorial/latest/03_Drawing_a_triangle/01_Presentation/01_Swap_chain.html)。DX12 路线按同一概念顺序建立映射，其对象与约束以 Microsoft 文档和本后端代码为准。49—54 课包含在基础/compute 之后加入的工程扩展。

[版本与资料说明](../../../../docs/REFERENCES.md) · [调试](../../../../docs/DEBUGGING.md) · [功能完成度](../../../../docs/FEATURE_MATRIX.md)

## 固定输入的实际 GPU 读回

![vk02_swapchain](../../../../docs/previews/vk02_swapchain.png)
