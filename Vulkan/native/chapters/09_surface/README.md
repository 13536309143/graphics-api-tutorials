# 第 09 课 · 窗口 Surface 与 HWND 连接 · Vulkan 原生

[54 课目录](../../COURSE.md) · [对照精讲](../../../../docs/compare/01_context_device.md) · [上一课](../08_logical_device/README.md) · [下一课](../10_swapchain/README.md)

## 本课目标与先修

学习目标：解释“窗口 Surface 与 HWND 连接”解决的问题，找到实际对象和参数，运行对应实验并用数据/画面证明。先完成前一课，并能说明它交付的对象与完成条件；本课在该依赖上增加新契约。

## 为什么需要这一层

窗口系统负责区域和输入，图形 API 的呈现连接负责让图像进入该窗口。离屏工作与可见呈现应拆成独立路径，使 compute 和自动验证不依赖桌面交互。

## Vulkan 原生 的对象、参数与执行推导

GLFW 用平台扩展创建 SurfaceKHR；其支持条件依赖 physical device 与 queue family。实验创建隐藏窗口并查询 surface 能力与可用格式/模式，不会假定所有图形队列都可呈现。

## 对照 DirectX 12：相同问题的另一种分工

通过 glfwGetWin32Window 取得 HWND，再创建 flip-discard 交换链。实验检查 buffer count 与 swap effect，并实际清屏/呈现一帧。窗口句柄不是 D3D12 resource，也不保存图像内容。

## 关键参数与可检查不变量

| 检查项 | Vulkan 表达 | DX12 表达 | 不变量 / 证据 |
| --- | --- | --- | --- |
| 平台句柄 | GLFW 创建 SurfaceKHR | GLFW 提供 HWND | 窗口不是纹理资源 |
| 呈现支持 | 对实际 surface 查询 family | 创建 DXGI window swapchain | graphics 能力不独自证明呈现成立 |
| 生命周期 | surface 晚于使用它的 swapchain | HWND 覆盖窗口使用期 | 先结束 GPU/呈现使用再释放依赖 |

先为每项指出源码中的创建/绑定位置，以及证明它成立的实际日志、字节或图像。数值中的设备限制以查询为准；固定实验尺寸不当作所有设备的上限。

## nvpro_core2 框架对照

非 headless 的 Application 需要 surface/swapchain 扩展；headless 目标仍可使用 RenderTarget 与图像读回，无需 UI。

这部分是原生 Vulkan 各课的框架对照，框架可运行入口仍为原有六课。如果本节需要新接入工作，会按正文说明；不把原生实验当作已完成的框架实现。

## 运行与实际检查

| 路线 | 实际实验 |
| --- | --- |
| Vulkan 原生 | [vk40_surface_support](../../lessons/vk40_surface_support/README.md) |
| DirectX 12 | [dx40_surface_support](../../../../DirectX12/lessons/dx40_surface_support/README.md) |

从合集根目录执行；普通图形程序 Esc 退出，设备/计算/诊断程序输出结果后结束。从“开始学习.cmd”进入本页时，表格可直接点击运行。

```powershell
cmake --preset vulkan-native
cmake --build --preset vulkan-native-debug --target vk40_surface_support --parallel 4
.\Vulkan\native\build\bin\Debug\vk40_surface_support.exe
ctest --preset vulkan-native-debug -R "^vk40_surface_support$" --output-on-failure
```

检查三个层次：创建/绑定/提交的接口约束；数据读回或目标画面；资源使用结束后的回收。诊断实验用合法信息或查询证明渠道可用，不故意保留 API 错误。图形 smoke 的像素检查与数值实验的逐项检查不同，具体实现见源码。

## 关键实现：从当前源码定位

来自 [Vulkan/native/common/advanced_labs.cpp](../../common/advanced_labs.cpp#L710)，第 710—753 行。下列是当前仓库的局部切片，可能在函数中间结束；完整上下文请打开文件，切片不作为单独可编译程序。

```cpp
void surfaceInspection() {
    require(glfwInit() != 0, "GLFW init failed");
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    auto* window = glfwCreateWindow(320, 240, "Surface inspection", nullptr, nullptr);
    require(window != nullptr, "Window failed");
    {
        NativeDevice owner(window);
        VkSurfaceCapabilitiesKHR caps{};
        vkCheck(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(owner.c.physical, owner.surface, &caps));
        uint32_t formats{}, modes{};
        vkCheck(vkGetPhysicalDeviceSurfaceFormatsKHR(
            owner.c.physical, owner.surface, &formats, nullptr));
        vkCheck(vkGetPhysicalDeviceSurfacePresentModesKHR(
            owner.c.physical, owner.surface, &modes, nullptr));
        require(formats > 0 && modes > 0, "Surface has no formats/present modes");
        std::cout << "Surface minImages=" << caps.minImageCount
                  << ", maxImages=" << caps.maxImageCount << ", formats=" << formats
                  << ", presentModes=" << modes << '\n';
    }
    glfwDestroyWindow(window);
    glfwTerminate();
}
// 整池重置必须发生在该代命令完成之后；每代写不同数据以发现旧命令残留。
void commandReuse(const VkContextRef& c) {
    VkBufferResource output;
    output.init(c, 256, VK_BUFFER_USAGE_TRANSFER_DST_BIT);
    VkCommandPoolCreateInfo pi{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    pi.queueFamilyIndex = c.family;
    VkCommandPool pool{};
    vkCheck(vkCreateCommandPool(c.device, &pi, nullptr, &pool));
    VkCommandBufferAllocateInfo ai{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    ai.commandPool = pool;
    ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    ai.commandBufferCount = 1;
    VkCommandBuffer cmd{};
    vkCheck(vkAllocateCommandBuffers(c.device, &ai, &cmd));
    VkFenceCreateInfo fi{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    VkFence fence{};
    vkCheck(vkCreateFence(c.device, &fi, nullptr, &fence));
    for (uint32_t generation = 0; generation < 4; ++generation) {
        vkCheck(vkResetCommandPool(c.device, pool, 0));
        VkCommandBufferBeginInfo bi{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
        vkCheck(vkBeginCommandBuffer(cmd, &bi));
```

阅读时按“前置对象 → 参数/布局 → 录制/调用 → 完成/可见性 → 释放”五步标注。共享 helper 减少重复代码，但不会取消每课的 API 契约。

## 受控修改实验

把同一段 buffer compute 放到没有窗口的程序里，说明它与呈现路径共享哪些对象。

先记录预测，再只改一个因素；保留基线与修改后的输出。不要同时改变 shader、资源范围和同步，这样即使结果变好也无法归因。

### 参考推导与通过标准

可以共享 device、queue、buffer、pipeline 和命令/同步原则；surface、swapchain 与平台输入不是必需条件。

## 常见失败与排查顺序

GLFW 默认创建 OpenGL context。显式 API 课程要设置 GLFW_NO_API，避免把两套上下文机制混在一起。

先看第一条错误和当前执行路径，再看实际数据/绑定，最后看 shader 与画面。需要 GPU 证据时抓取正常交互帧；仅四帧的 smoke 适合自动检查，不适合手动抓帧。

## 自测与本课交付

能否不看源码回答：把同一段 buffer compute 放到没有窗口的程序里，说明它与呈现路径共享哪些对象。

提交一份简短记录：输入与假设、唯一修改、预期、实际证据、原因、是否恢复基线。如果只能复述函数名而说不清何时允许访问/释放，先回到本课的数据流。

## 官方阅读位置与当前工程差异

[指定 Vulkan 教程对应位置](https://docs.vulkan.net.cn/tutorial/latest/03_Drawing_a_triangle/01_Presentation/00_Window_surface.html)。DX12 路线按同一概念顺序建立映射，其对象与约束以 Microsoft 文档和本后端代码为准。49—54 课包含在基础/compute 之后加入的工程扩展。

[版本与资料说明](../../../../docs/REFERENCES.md) · [调试](../../../../docs/DEBUGGING.md) · [功能完成度](../../../../docs/FEATURE_MATRIX.md)
