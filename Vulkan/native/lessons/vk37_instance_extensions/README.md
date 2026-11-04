# vk37_instance_extensions · 加载器/工厂与枚举

[54 课目录](../../COURSE.md)

## 目的与实际执行

SDK 中的头文件、loader 导入库、glslc 与 validation layer 有不同用途。安装更高版本头文件不会升级驱动。程序枚举 loader 版本和实例扩展，再核对物理设备支持；glslc 产物使用 SPIR-V，不能拿 GLSL 文本直接创建 shader module。

实例扩展用于窗口系统连接、debug utils 等实例层功能，设备扩展在创建设备时选择。glfwGetRequiredInstanceExtensions 返回当前平台 surface 所需扩展；无窗口计算不需要同一组 WSI 条件。pApplicationInfo 中的 apiVersion 不是“申请升级显卡”。

本程序有独立实验选择与验证路径，公共实现集中在 advanced_labs.cpp。设备/API 检查、数值 compute 与图形渲染采用不同证据；不是每个程序都会弹出窗口。

## 构建和运行

```powershell
cmake --preset vulkan-native
cmake --build --preset vulkan-native-debug --target vk37_instance_extensions --parallel 4
.\Vulkan\native\build\bin\Debug\vk37_instance_extensions.exe
```

## 深入学习：数据、接口与执行

- [第 02 课：工具链、SDK、依赖与可复现构建](../../chapters/02_environment/README.md)
- [第 04 课：实例扩展与 DXGI 工厂](../../chapters/04_instance_factory/README.md)

### 本课的真实源码切片

来自 [Vulkan/native/common/advanced_labs.cpp](../../common/advanced_labs.cpp#L775)，第 775—818 行；这是现有实现的局部摘录，末尾可能处于函数中间，不是可单独编译的完整程序。

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

### 同主题跨 API 导航

| 路线 | 程序 |
| --- | --- |
| Vulkan 原生 | [vk37_instance_extensions](README.md) |
| 对照后端 | [dx37_instance_extensions](../../../../DirectX12/lessons/dx37_instance_extensions/README.md) |

### 先预测，再验证

故意在副本中改变运行工作目录，再从命令行启动程序，说明为何它仍应能找到 shader/assets。

参考推导：资源以 executable 所在目录为起点，构建复制到旁边。若只有 IDE 中能运行，检查是否偷偷依赖了工作目录或本机绝对路径。

把某个窗口相关要求从无窗口计算路径中移除，先说明哪些操作因此不再允许，再解释为什么计算仍能成立。

参考推导：没有 surface/swapchain 便不能走窗口呈现，但 compute pipeline、buffer 和 queue submit 不依赖可见窗口。需求应与实际功能一致。

## 验证与边界

运行 `--smoke` 执行有限自动检查。数值实验读取实际 GPU 结果，图形实验检查非空图像并可查看读回。交互重建与性能还需额外证据；窗口 renderer 当前保留串行等待，双槽计算实验不会被描述为完整多帧呈现引擎。
