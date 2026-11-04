# vk38_diagnostic_delivery · 诊断消息链路

[54 课目录](../../COURSE.md)

## 目的与实际执行

枚举 layer 后创建 debug messenger，并按 severity/type 分类。实验额外注册 INFO 回调，提交应用信息并确认回调收到。外部 loader manifest 诊断与应用 VUID 分别记录。GPU 区域标签通过 debug-utils entry points 录制。

本程序有独立实验选择与验证路径，公共实现集中在 advanced_labs.cpp。设备/API 检查、数值 compute 与图形渲染采用不同证据；不是每个程序都会弹出窗口。

## 构建和运行

```powershell
cmake --preset vulkan-native
cmake --build --preset vulkan-native-debug --target vk38_diagnostic_delivery --parallel 4
.\Vulkan\native\build\bin\Debug\vk38_diagnostic_delivery.exe
```

## 深入学习：数据、接口与执行

- [第 05 课：验证层、调试队列与诊断证据](../../chapters/05_validation/README.md)

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
| Vulkan 原生 | [vk38_diagnostic_delivery](README.md) |
| 对照后端 | [dx38_diagnostic_delivery](../../../../DirectX12/lessons/dx38_diagnostic_delivery/README.md) |

### 先预测，再验证

注入一条 INFO 应用消息，确认它会进入日志/回调而不会增加 API 错误计数。

参考推导：消息严重度与错误计数策略独立。合法的应用信息用于测试诊断渠道，不应冒充 validation error 或通过清空错误队列让检查成功。

## 验证与边界

运行 `--smoke` 执行有限自动检查。数值实验读取实际 GPU 结果，图形实验检查非空图像并可查看读回。交互重建与性能还需额外证据；窗口 renderer 当前保留串行等待，双槽计算实验不会被描述为完整多帧呈现引擎。
