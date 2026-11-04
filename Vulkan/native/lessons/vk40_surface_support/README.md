# vk40_surface_support · 窗口呈现能力

[54 课目录](../../COURSE.md)

## 目的与实际执行

GLFW 用平台扩展创建 SurfaceKHR；其支持条件依赖 physical device 与 queue family。实验创建隐藏窗口并查询 surface 能力与可用格式/模式，不会假定所有图形队列都可呈现。

检查 min/max image count、currentExtent、supportedUsageFlags、surface formats 与 present modes。maxImageCount=0 表示没有给出上限，不是零张图像。当前例子选择 FIFO，并要求读回/blit 用途；这会比只呈现的最小程序需求更多。

处理 OUT_OF_DATE/SUBOPTIMAL 以及 framebuffer 尺寸变化。cleanup 等 GPU 后释放 scene/depth/MSAA/view/semaphore，重新创建，并重新执行实验的资源配置回调。格式变化可能影响 pipeline，不能永远假定只有 extent 改变。

本程序有独立实验选择与验证路径，公共实现集中在 advanced_labs.cpp。设备/API 检查、数值 compute 与图形渲染采用不同证据；不是每个程序都会弹出窗口。

## 构建和运行

```powershell
cmake --preset vulkan-native
cmake --build --preset vulkan-native-debug --target vk40_surface_support --parallel 4
.\Vulkan\native\build\bin\Debug\vk40_surface_support.exe
```

## 深入学习：数据、接口与执行

- [第 09 课：窗口 Surface 与 HWND 连接](../../chapters/09_surface/README.md)
- [第 10 课：交换链选择、格式与呈现模式](../../chapters/10_swapchain/README.md)
- [第 22 课：窗口变化与交换链重建](../../chapters/22_swapchain_recreation/README.md)

### 本课的真实源码切片

来自 [Vulkan/native/common/advanced_labs.cpp](../../common/advanced_labs.cpp#L710)，第 710—753 行；这是现有实现的局部摘录，末尾可能处于函数中间，不是可单独编译的完整程序。

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

### 同主题跨 API 导航

| 路线 | 程序 |
| --- | --- |
| Vulkan 原生 | [vk40_surface_support](README.md) |
| 对照后端 | [dx40_surface_support](../../../../DirectX12/lessons/dx40_surface_support/README.md) |

### 先预测，再验证

把同一段 buffer compute 放到没有窗口的程序里，说明它与呈现路径共享哪些对象。

参考推导：可以共享 device、queue、buffer、pipeline 和命令/同步原则；surface、swapchain 与平台输入不是必需条件。

解释为何两帧 CPU 资源槽与三张交换链图像可以同时存在。

参考推导：前者跟踪应用可变资源的在途使用期，后者由 acquire/呈现系统选择。两个索引各有复用条件，不能靠数值恰好相等统一。

普通交互模式下反复拖动、最小化、恢复窗口，记录会触发哪些重建路径。

参考推导：应避免零尺寸分配/除零和旧图像引用。已有 smoke 检查有限帧输出，反复 resize 属于额外手动验收，不能混写成已自动覆盖所有组合。

## 验证与边界

运行 `--smoke` 执行有限自动检查。数值实验读取实际 GPU 结果，图形实验检查非空图像并可查看读回。交互重建与性能还需额外证据；窗口 renderer 当前保留串行等待，双槽计算实验不会被描述为完整多帧呈现引擎。
