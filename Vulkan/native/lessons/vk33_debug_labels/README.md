# vk33_debug_labels · GPU 调试标签

[54 课目录](../../COURSE.md)

## 本课独立工程与关键注释

[main.cpp](main.cpp) · [本地辅助实现](support/) · [本课着色器](shaders/) · [CMakeLists.txt](CMakeLists.txt)

本目录包含独立课程源码，关键位置有中文注释。单独复制整个文件夹后可用 `cmake -S . -B build -A x64` 配置、`cmake --build build --config Debug` 构建；不需要其他章节源码。

## 目的与实际执行

枚举 layer 后创建 debug messenger，并按 severity/type 分类。实验额外注册 INFO 回调，提交应用信息并确认回调收到。外部 loader manifest 诊断与应用 VUID 分别记录。GPU 区域标签通过 debug-utils entry points 录制。

本程序的 C++、辅助实现、着色器和构建脚本均位于本目录，关键步骤含中文注释。设备/API 检查、数值 compute 与图形渲染采用不同证据；不是每个程序都会弹出窗口。

## 构建和运行

```powershell
cmake --preset vulkan-native
cmake --build --preset vulkan-native-debug --target vk33_debug_labels --parallel 4
.\Vulkan\native\build\bin\Debug\vk33_debug_labels.exe
```

## 深入学习：数据、接口与执行

- [第 05 课：验证层、调试队列与诊断证据](../../chapters/05_validation/README.md)

### 本课的真实源码切片

来自 [Vulkan/native/lessons/vk33_debug_labels/main.cpp](main.cpp#L13)，第 13—46 行；这是现有实现的局部摘录，末尾可能处于函数中间，不是可单独编译的完整程序。

```cpp
int runChapter(int argc, char** argv) {
    auto options = labOptions(argc, argv, 33);
    nativeShaders = options.directory / "shaders";
    validationErrors = 0;
    {
        NativeDevice owner(nullptr, false);
        const auto& c = owner.c;
        {
            auto begin = reinterpret_cast<PFN_vkCmdBeginDebugUtilsLabelEXT>(
                vkGetDeviceProcAddr(c.device, "vkCmdBeginDebugUtilsLabelEXT"));
            auto end = reinterpret_cast<PFN_vkCmdEndDebugUtilsLabelEXT>(
                vkGetDeviceProcAddr(c.device, "vkCmdEndDebugUtilsLabelEXT"));
            require(begin && end, "Debug label entry points missing");
            immediate(c, [&](VkCommandBuffer cmd) {
                VkDebugUtilsLabelEXT label{VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT};
                label.pLabelName = "Course/Upload pass";
                label.color[1] = 1;
                begin(cmd, &label);
                end(cmd);
            });
            std::cout << "Submitted named GPU debug region\n";
        }
    }
    std::cout << "Validation errors: " << validationErrors << '\n';
    return validationErrors ? 1 : 0;
}
} // namespace course

int main(int argc, char** argv) try {
    return course::runChapter(argc, argv);
} catch (const std::exception& error) {
    std::cerr << "ERROR: " << error.what() << '\n';
    return 1;
}
```

### 同主题跨 API 导航

| 路线 | 程序 |
| --- | --- |
| Vulkan 原生 | [vk33_debug_labels](README.md) |
| 对照后端 | [dx33_debug_labels](../../../../DirectX12/lessons/dx33_debug_labels/README.md) |

### 先预测，再验证

注入一条 INFO 应用消息，确认它会进入日志/回调而不会增加 API 错误计数。

参考推导：消息严重度与错误计数策略独立。合法的应用信息用于测试诊断渠道，不应冒充 validation error 或通过清空错误队列让检查成功。

## 验证与边界

运行 `--smoke` 执行有限自动检查。数值实验读取实际 GPU 结果，图形实验检查非空图像并可查看读回。交互重建与性能还需额外证据；窗口 renderer 当前保留串行等待，双槽计算实验不会被描述为完整多帧呈现引擎。
