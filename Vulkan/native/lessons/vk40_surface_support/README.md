# vk40_surface_support · 窗口呈现能力

[54 课目录](../../COURSE.md)

## 本课独立工程与关键注释

[main.cpp](main.cpp) · [本地辅助实现](support/) · [本课着色器](shaders/) · [CMakeLists.txt](CMakeLists.txt)

本目录包含独立课程源码，关键位置有中文注释。单独复制整个文件夹后可用 `cmake -S . -B build -A x64` 配置、`cmake --build build --config Debug` 构建；不需要其他章节源码。

## 目的与实际执行

GLFW 用平台扩展创建 SurfaceKHR；其支持条件依赖 physical device 与 queue family。实验创建隐藏窗口并查询 surface 能力与可用格式/模式，不会假定所有图形队列都可呈现。

检查 min/max image count、currentExtent、supportedUsageFlags、surface formats 与 present modes。maxImageCount=0 表示没有给出上限，不是零张图像。当前例子选择 FIFO，并要求读回/blit 用途；这会比只呈现的最小程序需求更多。

处理 OUT_OF_DATE/SUBOPTIMAL 以及 framebuffer 尺寸变化。cleanup 等 GPU 后释放 scene/depth/MSAA/view/semaphore，重新创建，并重新执行实验的资源配置回调。格式变化可能影响 pipeline，不能永远假定只有 extent 改变。

本程序的 C++、辅助实现、着色器和构建脚本均位于本目录，关键步骤含中文注释。设备/API 检查、数值 compute 与图形渲染采用不同证据；不是每个程序都会弹出窗口。

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

来自 [Vulkan/native/lessons/vk40_surface_support/main.cpp](main.cpp#L41)，第 41—57 行；这是现有实现的局部摘录，末尾可能处于函数中间，不是可单独编译的完整程序。

```cpp
int runChapter(int argc, char** argv) {
    auto options = labOptions(argc, argv, 40);
    nativeShaders = options.directory / "shaders";
    validationErrors = 0;
    {
        surfaceInspection();
        return validationErrors ? 1 : 0;
    }
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
