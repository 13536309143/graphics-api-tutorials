# vk01_instance_device · 实例、设备与队列

[路线首页](../../README.md) · [上一课](../../README.md) · [下一课](../vk02_swapchain/README.md)

## 构建和运行

从仓库根目录执行，完成配置后后续只需 build：

```powershell
cmake --preset vulkan-native
cmake --build --preset vulkan-native-debug --target vk01_instance_device --parallel 4
.\Vulkan\native\build\bin\Debug\vk01_instance_device.exe
```

也可以通过根目录“开始学习.cmd”打开页面后点击“运行本课”。01/08 是控制台计算/设备检查示例，点击后可用“查看运行输出”阅读结果。其余示例 Esc 退出，拖动窗口可测试尺寸重建。

预期结果：只输出显卡信息；没有窗口，这是设备初始化课。

## 先读哪些代码

NativeDevice：instance → validation messenger → physical device/features → queue family → logical device。

本目录 [main.cpp](main.cpp) 选择本课的执行路径。完整 API 实现在 `common/native_app.hpp 与 ../common/scene_vk.hpp`（相对路线根目录），数学和网格在仓库 shared/scene.hpp。每课都是独立程序；共享代码避免把相同资源生命周期错误复制十遍，也会为后续课预先初始化部分公共状态。首次阅读只沿本课列出的调用路径展开。

## 原理和实现

Vulkan 区分实例、物理设备和逻辑设备。实例负责与驱动体系交互，物理设备代表适配器，逻辑设备启用选定特性并提供队列。本课要求 Vulkan 1.3、dynamicRendering、synchronization2，并选择同时支持 graphics/compute 的队列族；窗口课程还检查 present。SDK 版本不等于驱动能提供的 API 版本，必须查询设备。

## 修改练习

枚举多个设备并打印队列族；让选择策略偏向独立显卡。

修改 shader 后重新构建，再运行。Vulkan GLSL 会被编译并复制到 exe 旁的 shaders；DX12 的 HLSL 副本也随构建更新，运行时再编译。应修改源目录而不是运行副本。

## 自测

为何队列句柄必须来自创建逻辑设备时申请的队列族？

参考：逻辑设备只创建所申请的队列，不能随意拿另一个族的索引使用。


## 验证和调试

运行 `vk01_instance_device.exe --smoke` 执行四帧（或设备/计算检查）并退出；颜色课会从 GPU 读回 PPM，计算课验证 256 个数。参见仓库 [调试指南](../../../../docs/DEBUGGING.md)。这些基础例子按帧串行等待，重点是可追踪的生命周期；多帧重叠不能仅删除 wait，需要同时改成帧资源环。


## 深入学习：数据、接口与执行

没有窗口时不需 surface/交换链。本课检查 API 版本、特性与 graphics/compute 队列要求；physical device 是能力来源，logical device 是启用需求后的操作入口。

配套精讲：[三 API 对照 · 窗口、设备与队列](../../../../docs/compare/01_context_device.md)。先读概念与参数选择，再返回本课实现核对，不必一次展开所有公共类。

### 本课的真实源码切片

来自 [Vulkan/native/common/native_app.hpp](../../common/native_app.hpp#L29)，第 29—58 行；这是现有实现的局部摘录，末尾可能处于函数中间，不是可单独编译的完整程序。

```cpp
    NativeDevice(GLFWwindow* window, bool timeline = false) {
        uint32_t count = 0;
        std::vector<const char*> extensions;
        if (window) {
            const char** required = glfwGetRequiredInstanceExtensions(&count);
            extensions.assign(required, required + count);
        }
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        uint32_t layerCount = 0;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
        std::vector<VkLayerProperties> layers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, layers.data());
        bool debug = std::any_of(layers.begin(), layers.end(), [](const auto& x) {
            return std::strcmp(x.layerName, "VK_LAYER_KHRONOS_validation") == 0;
        });
        const char* layer = "VK_LAYER_KHRONOS_validation";
        if (!debug)
            std::cerr << "Validation layer not installed; rendering remains available\n";
        VkApplicationInfo app{VK_STRUCTURE_TYPE_APPLICATION_INFO};
        app.pApplicationName = "Native Vulkan Course";
        app.apiVersion = VK_API_VERSION_1_3;
        VkDebugUtilsMessengerCreateInfoEXT di{
            VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
        di.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
        di.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                         VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                         VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        di.pfnUserCallback = validation;
        VkInstanceCreateInfo info{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
```

### 同主题跨 API 导航

以下是同主题参考；场景、功能完成度和版本基线可能不同，具体边界见对照章节。

| 路线 | 可运行参照 / 当前范围 |
| --- | --- |
| OpenGL | [01_window](../../../../OpenGL/lessons/01_window/README.md) |
| Vulkan 原生 | [vk01_instance_device](README.md) |
| Vulkan / nvpro_core2 | [nv01_context](../../../nvpro/lessons/nv01_context/README.md) |
| DirectX 12 | [dx01_device](../../../../DirectX12/lessons/dx01_device/README.md) |

### 先预测，再验证

查询到 dynamicRendering 支持，就能不启用直接使用吗？

参考推导：查询与启用是不同步骤。沿特性链看创建设备所声明的需求，并在不满足时明确失败。

在一次实验中只改一个因素，记录预测、实际画面/日志和原因。检查已有 smoke 是否仍通过；图形调试器里再核对本次 draw 的实际输入/输出，不能仅凭进程正常退出下结论。
