# 第 02 课 · 工具链、SDK、依赖与可复现构建 · Vulkan 原生

[54 课目录](../../COURSE.md) · [对照精讲](../../../../docs/compare/01_context_device.md) · [上一课](../01_architecture/README.md) · [下一课](../03_lifetime/README.md)

## 本课目标与先修

学习目标：解释“工具链、SDK、依赖与可复现构建”解决的问题，找到实际对象和参数，运行对应实验并用数据/画面证明。先完成前一课，并能说明它交付的对象与完成条件；本课在该依赖上增加新契约。

## 为什么需要这一层

把配置、编译、链接和运行四个阶段分开。CMake configure 选择编译器与依赖；build 产生 CPU 程序和 shader 产物；链接解决符号；运行还依赖驱动、资源路径和设备能力。某一步成功不能证明后续全部成立。

本工程固定源码依赖与哈希，离线 HTML 不依赖 Python。首次可联网准备缓存，再按路线独立 preset 构建。不要用一个 build 目录混用根 CMake 与子目录 source root。Debug 用于断点和验证，Release 用于经过控制的性能测量。

## Vulkan 原生 的对象、参数与执行推导

SDK 中的头文件、loader 导入库、glslc 与 validation layer 有不同用途。安装更高版本头文件不会升级驱动。程序枚举 loader 版本和实例扩展，再核对物理设备支持；glslc 产物使用 SPIR-V，不能拿 GLSL 文本直接创建 shader module。

## 对照 DirectX 12：相同问题的另一种分工

Windows SDK 提供头文件/导入库；本课程 HLSL 使用 D3DCompiler/SM5.1，未切换 DXC/SM6。DXGI 枚举 adapter 能看到硬件与软件实现；运行时 feature query 才是功能选择依据，不能只看 Visual Studio 是否识别接口名称。

## 关键参数与可检查不变量

| 检查项 | Vulkan 表达 | DX12 表达 | 不变量 / 证据 |
| --- | --- | --- | --- |
| CPU 工具链 | C++20 / glslc / SPIR-V | C++20 / D3DCompiler / SM5.1 | 编译目标、运行产物和实际路径对应 |
| 能力来源 | loader 与 physical device 查询 | DXGI 枚举与 feature query | 头文件版本不升级 GPU 驱动 |
| 构建隔离 | vulkan-native preset | dx12 preset | 各自 build 缓存，不混用 source root |

先为每项指出源码中的创建/绑定位置，以及证明它成立的实际日志、字节或图像。数值中的设备限制以查询为准；固定实验尺寸不当作所有设备的上限。

## nvpro_core2 框架对照

固定子模块提交和配套 Vulkan headers/Volk，按依赖文档准备。框架主分支的最新接口与本项目固定版本可能不同，不直接混接。

这部分是原生 Vulkan 各课的框架对照，框架可运行入口仍为原有六课。如果本节需要新接入工作，会按正文说明；不把原生实验当作已完成的框架实现。

## 运行与实际检查

| 路线 | 实际实验 |
| --- | --- |
| Vulkan 原生 | [vk37_instance_extensions](../../lessons/vk37_instance_extensions/README.md) |
| DirectX 12 | [dx37_instance_extensions](../../../../DirectX12/lessons/dx37_instance_extensions/README.md) |

从合集根目录执行；普通图形程序 Esc 退出，设备/计算/诊断程序输出结果后结束。从“开始学习.cmd”进入本页时，表格可直接点击运行。

```powershell
cmake --preset vulkan-native
cmake --build --preset vulkan-native-debug --target vk37_instance_extensions --parallel 4
.\Vulkan\native\build\bin\Debug\vk37_instance_extensions.exe
ctest --preset vulkan-native-debug -R "^vk37_instance_extensions$" --output-on-failure
```

检查三个层次：创建/绑定/提交的接口约束；数据读回或目标画面；资源使用结束后的回收。诊断实验用合法信息或查询证明渠道可用，不故意保留 API 错误。图形 smoke 的像素检查与数值实验的逐项检查不同，具体实现见源码。

## 关键实现：从当前源码定位

来自 [Vulkan/native/common/advanced_labs.cpp](../../common/advanced_labs.cpp#L775)，第 775—818 行。下列是当前仓库的局部切片，可能在函数中间结束；完整上下文请打开文件，切片不作为单独可编译程序。

```cpp
void inspection(const VkContextRef& c, int id) {
    if (id == 37) {
        uint32_t version{};
        vkCheck(vkEnumerateInstanceVersion(&version));
        uint32_t count{};
        vkCheck(vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr));
        std::vector<VkExtensionProperties> extensions(count);
        vkCheck(vkEnumerateInstanceExtensionProperties(nullptr, &count, extensions.data()));
        bool found = false;
        for (const auto& e : extensions) {
            std::cout << e.extensionName << '\n';
            found |= std::strcmp(e.extensionName, VK_EXT_DEBUG_UTILS_EXTENSION_NAME) == 0;
        }
        require(found, "Debug utils extension missing");
        std::cout << "Loader version=" << VK_API_VERSION_MAJOR(version) << '.'
                  << VK_API_VERSION_MINOR(version) << '\n';
    } else if (id == 38) {
        uint32_t layers{};
        vkCheck(vkEnumerateInstanceLayerProperties(&layers, nullptr));
        std::vector<VkLayerProperties> list(layers);
        vkCheck(vkEnumerateInstanceLayerProperties(&layers, list.data()));
        for (const auto& l : list)
            std::cout << "layer=" << l.layerName << '\n';
        int delivered = 0;
        VkDebugUtilsMessengerCreateInfoEXT info{
            VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
        info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;
        info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT;
        info.pUserData = &delivered;
        info.pfnUserCallback = [](VkDebugUtilsMessageSeverityFlagBitsEXT,
                                  VkDebugUtilsMessageTypeFlagsEXT,
                                  const VkDebugUtilsMessengerCallbackDataEXT*,
                                  void* user) -> VkBool32 {
            ++*static_cast<int*>(user);
            return VK_FALSE;
        };
        auto create = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
            vkGetInstanceProcAddr(c.instance, "vkCreateDebugUtilsMessengerEXT"));
        auto destroy = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
            vkGetInstanceProcAddr(c.instance, "vkDestroyDebugUtilsMessengerEXT"));
        auto message = reinterpret_cast<PFN_vkSubmitDebugUtilsMessageEXT>(
            vkGetInstanceProcAddr(c.instance, "vkSubmitDebugUtilsMessageEXT"));
        require(create && destroy && message, "Debug utils functions missing");
        VkDebugUtilsMessengerEXT messenger{};
```

阅读时按“前置对象 → 参数/布局 → 录制/调用 → 完成/可见性 → 释放”五步标注。共享 helper 减少重复代码，但不会取消每课的 API 契约。

## 受控修改实验

故意在副本中改变运行工作目录，再从命令行启动程序，说明为何它仍应能找到 shader/assets。

先记录预测，再只改一个因素；保留基线与修改后的输出。不要同时改变 shader、资源范围和同步，这样即使结果变好也无法归因。

### 参考推导与通过标准

资源以 executable 所在目录为起点，构建复制到旁边。若只有 IDE 中能运行，检查是否偷偷依赖了工作目录或本机绝对路径。

## 常见失败与排查顺序

链接缺库应查 CMake target 和架构；shader 缺文件查复制与路径；设备版本不足查驱动/能力。不要用复制任意 DLL 同时处理三类故障。

先看第一条错误和当前执行路径，再看实际数据/绑定，最后看 shader 与画面。需要 GPU 证据时抓取正常交互帧；仅四帧的 smoke 适合自动检查，不适合手动抓帧。

## 自测与本课交付

能否不看源码回答：故意在副本中改变运行工作目录，再从命令行启动程序，说明为何它仍应能找到 shader/assets。

提交一份简短记录：输入与假设、唯一修改、预期、实际证据、原因、是否恢复基线。如果只能复述函数名而说不清何时允许访问/释放，先回到本课的数据流。

## 官方阅读位置与当前工程差异

[指定 Vulkan 教程对应位置](https://docs.vulkan.net.cn/tutorial/latest/02_Development_environment.html)。DX12 路线按同一概念顺序建立映射，其对象与约束以 Microsoft 文档和本后端代码为准。49—54 课包含在基础/compute 之后加入的工程扩展。

[版本与资料说明](../../../../docs/REFERENCES.md) · [调试](../../../../docs/DEBUGGING.md) · [功能完成度](../../../../docs/FEATURE_MATRIX.md)
