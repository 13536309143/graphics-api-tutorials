# dx40_surface_support · 窗口呈现能力

[54 课目录](../../COURSE.md)

## 本课独立工程与关键注释

[main.cpp](main.cpp) · [本地辅助实现](support/) · [本课着色器](shaders/) · [CMakeLists.txt](CMakeLists.txt)

本目录包含独立课程源码，关键位置有中文注释。单独复制整个文件夹后可用 `cmake -S . -B build -A x64` 配置、`cmake --build build --config Debug` 构建；不需要其他章节源码。

## 目的与实际执行

通过 glfwGetWin32Window 取得 HWND，再创建 flip-discard 交换链。实验检查 buffer count 与 swap effect，并实际清屏/呈现一帧。窗口句柄不是 D3D12 resource，也不保存图像内容。

课程选择两个 RGBA8 flip-discard back buffer，GetCurrentBackBufferIndex 决定本帧目标。显示同步与 tearing 的支持/flag 要另行检查，不能仅把 Present 参数设为零就称作公平性能测试。

ResizeBuffers 前释放所有 back buffer 引用并等待相关 GPU 工作；随后重建 RTV/DSV、depth 与 MSAA/离屏目标。只释放一个局部 ComPtr 而仍有其他引用，不满足重建要求。

本程序的 C++、辅助实现、着色器和构建脚本均位于本目录，关键步骤含中文注释。设备/API 检查、数值 compute 与图形渲染采用不同证据；不是每个程序都会弹出窗口。

## 构建和运行

```powershell
cmake --preset dx12
cmake --build --preset dx12-debug --target dx40_surface_support --parallel 4
.\DirectX12\build\bin\Debug\dx40_surface_support.exe
```

## 深入学习：数据、接口与执行

- [第 09 课：窗口 Surface 与 HWND 连接](../../chapters/09_surface/README.md)
- [第 10 课：交换链选择、格式与呈现模式](../../chapters/10_swapchain/README.md)
- [第 22 课：窗口变化与交换链重建](../../chapters/22_swapchain_recreation/README.md)

### 本课的真实源码切片

来自 [DirectX12/lessons/dx40_surface_support/main.cpp](main.cpp#L39)，第 39—53 行；这是现有实现的局部摘录，末尾可能处于函数中间，不是可单独编译的完整程序。

```cpp
int runChapter(int argc, char** argv) {
    auto options = labOptions(argc, argv, 40);
    {
        surfaceInspection(options);
        return 0;
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
| DirectX 12 | [dx40_surface_support](README.md) |
| 对照后端 | [vk40_surface_support](../../../Vulkan/native/lessons/vk40_surface_support/README.md) |

### 先预测，再验证

把同一段 buffer compute 放到没有窗口的程序里，说明它与呈现路径共享哪些对象。

参考推导：可以共享 device、queue、buffer、pipeline 和命令/同步原则；surface、swapchain 与平台输入不是必需条件。

解释为何两帧 CPU 资源槽与三张交换链图像可以同时存在。

参考推导：前者跟踪应用可变资源的在途使用期，后者由 acquire/呈现系统选择。两个索引各有复用条件，不能靠数值恰好相等统一。

普通交互模式下反复拖动、最小化、恢复窗口，记录会触发哪些重建路径。

参考推导：应避免零尺寸分配/除零和旧图像引用。已有 smoke 检查有限帧输出，反复 resize 属于额外手动验收，不能混写成已自动覆盖所有组合。

## 验证与边界

运行 `--smoke` 执行有限自动检查。数值实验读取实际 GPU 结果，图形实验检查非空图像并可查看读回。交互重建与性能还需额外证据；窗口 renderer 当前保留串行等待，双槽计算实验不会被描述为完整多帧呈现引擎。
