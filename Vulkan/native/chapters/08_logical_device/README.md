# 第 08 课 · 逻辑设备、特性链与基础完成条件 · Vulkan 原生

[54 课目录](../../COURSE.md) · [对照精讲](../../../../docs/compare/01_context_device.md) · [上一课](../07_queues/README.md) · [下一课](../09_surface/README.md)

## 本课目标与先修

学习目标：解释“逻辑设备、特性链与基础完成条件”解决的问题，找到实际对象和参数，运行对应实验并用数据/画面证明。先完成前一课，并能说明它交付的对象与完成条件；本课在该依赖上增加新契约。

## 为什么需要这一层

能力查询与特性启用属于两个步骤。查询回答能做什么，创建回答本应用声明要做什么。C++ 结构清零、必要类型字段、链中对象的存活范围都属于正确初始化的一部分。

## Vulkan 原生 的对象、参数与执行推导

设备创建只启用实际使用的 dynamicRendering/synchronization2；timeline 实验额外查询并启用 Vulkan 1.2 的 timelineSemaphore。pNext 链的结构必须正确且在调用期间有效。获取 Queue 后仍要创建命令和同步对象才能执行工作。

## 对照 DirectX 12：相同问题的另一种分工

Device 创建后以 feature query 选择后续路径。CreateCommandQueue、CreateCommandAllocator、CreateCommandList 和 CreateFence 分别建立提交、命令存储、录制与完成计数。它们的存在不代表已经执行任何 GPU 指令。

## 关键参数与可检查不变量

| 检查项 | Vulkan 表达 | DX12 表达 | 不变量 / 证据 |
| --- | --- | --- | --- |
| 查询与启用 | features2 查询，create chain 启用 | feature query 后选择路径 | 应用需求不能超出支持集合 |
| 基础特性 | dynamicRendering / synchronization2 | D3D12 基础对象 | 新效果仍需额外能力判断 |
| 计数同步特性 | timelineSemaphore 按实验启用 | fence 原生计数 | Vulkan timeline 不替代 WSI binary 约定 |

先为每项指出源码中的创建/绑定位置，以及证明它成立的实际日志、字节或图像。数值中的设备限制以查询为准；固定实验尺寸不当作所有设备的上限。

## nvpro_core2 框架对照

Context 汇总这些创建步骤。阅读框架时关注需求到启用链的转换及错误处理，不要求逐次抄写所有结构体。

这部分是原生 Vulkan 各课的框架对照，框架可运行入口仍为原有六课。如果本节需要新接入工作，会按正文说明；不把原生实验当作已完成的框架实现。

## 运行与实际检查

| 路线 | 实际实验 |
| --- | --- |
| Vulkan 原生 | [vk01_instance_device](../../lessons/vk01_instance_device/README.md) |
| DirectX 12 | [dx01_device](../../../../DirectX12/lessons/dx01_device/README.md) |

从合集根目录执行；普通图形程序 Esc 退出，设备/计算/诊断程序输出结果后结束。从“开始学习.cmd”进入本页时，表格可直接点击运行。

```powershell
cmake --preset vulkan-native
cmake --build --preset vulkan-native-debug --target vk01_instance_device --parallel 4
.\Vulkan\native\build\bin\Debug\vk01_instance_device.exe
ctest --preset vulkan-native-debug -R "^vk01_instance_device$" --output-on-failure
```

检查三个层次：创建/绑定/提交的接口约束；数据读回或目标画面；资源使用结束后的回收。诊断实验用合法信息或查询证明渠道可用，不故意保留 API 错误。图形 smoke 的像素检查与数值实验的逐项检查不同，具体实现见源码。

## 关键实现：从当前源码定位

来自 [Vulkan/native/common/native_app.hpp](../../common/native_app.hpp#L29)，第 29—72 行。下列是当前仓库的局部切片，可能在函数中间结束；完整上下文请打开文件，切片不作为单独可编译程序。

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
        info.pApplicationInfo = &app;
        info.enabledExtensionCount = uint32_t(extensions.size());
        info.ppEnabledExtensionNames = extensions.data();
        if (debug) {
            info.enabledLayerCount = 1;
            info.ppEnabledLayerNames = &layer;
            info.pNext = &di;
        }
        vkCheck(vkCreateInstance(&info, nullptr, &c.instance));
        auto create = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
            vkGetInstanceProcAddr(c.instance, "vkCreateDebugUtilsMessengerEXT"));
        if (debug && create)
            vkCheck(create(c.instance, &di, nullptr, &messenger));
        if (window)
```

阅读时按“前置对象 → 参数/布局 → 录制/调用 → 完成/可见性 → 释放”五步标注。共享 helper 减少重复代码，但不会取消每课的 API 契约。

## 受控修改实验

在特性查询和启用处分别打断点，记录同一特性的两次值和来源。

先记录预测，再只改一个因素；保留基线与修改后的输出。不要同时改变 shader、资源范围和同步，这样即使结果变好也无法归因。

### 参考推导与通过标准

查询结构的 true 来自驱动能力，启用结构的 true 来自应用选择；后者不能超出前者。直接把整个查询结果原样启用会扩大需求并模糊契约。

## 常见失败与排查顺序

复用一段过期 pNext 指针或忘记 sType，会在逻辑上尚未进入绘制之前就破坏调用参数。

先看第一条错误和当前执行路径，再看实际数据/绑定，最后看 shader 与画面。需要 GPU 证据时抓取正常交互帧；仅四帧的 smoke 适合自动检查，不适合手动抓帧。

## 自测与本课交付

能否不看源码回答：在特性查询和启用处分别打断点，记录同一特性的两次值和来源。

提交一份简短记录：输入与假设、唯一修改、预期、实际证据、原因、是否恢复基线。如果只能复述函数名而说不清何时允许访问/释放，先回到本课的数据流。

## 官方阅读位置与当前工程差异

[指定 Vulkan 教程对应位置](https://docs.vulkan.net.cn/tutorial/latest/03_Drawing_a_triangle/00_Setup/04_Logical_device_and_queues.html)。DX12 路线按同一概念顺序建立映射，其对象与约束以 Microsoft 文档和本后端代码为准。49—54 课包含在基础/compute 之后加入的工程扩展。

[版本与资料说明](../../../../docs/REFERENCES.md) · [调试](../../../../docs/DEBUGGING.md) · [功能完成度](../../../../docs/FEATURE_MATRIX.md)
