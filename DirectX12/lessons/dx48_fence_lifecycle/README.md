# dx48_fence_lifecycle · 完成标记生命周期

[54 课目录](../../COURSE.md)

## 本课独立工程与关键注释

[main.cpp](main.cpp) · [本地辅助实现](support/) · [本课着色器](shaders/) · [CMakeLists.txt](CMakeLists.txt)

本目录包含独立课程源码，关键位置有中文注释。单独复制整个文件夹后可用 `cmake -S . -B build -A x64` 配置、`cmake --build build --config Debug` 构建；不需要其他章节源码。

## 目的与实际执行

back buffer 在 PRESENT/COMMON 与渲染用途间转换，Present 与 queue fence 各司其职。DX12 fence 是单调计数，不能套用 Vulkan binary fence 的 reset 模型；实验连续推进完成值并检查。

本程序的 C++、辅助实现、着色器和构建脚本均位于本目录，关键步骤含中文注释。设备/API 检查、数值 compute 与图形渲染采用不同证据；不是每个程序都会弹出窗口。

## 构建和运行

```powershell
cmake --preset dx12
cmake --build --preset dx12-debug --target dx48_fence_lifecycle --parallel 4
.\DirectX12\build\bin\Debug\dx48_fence_lifecycle.exe
```

## 深入学习：数据、接口与执行

- [第 20 课：获取、提交、呈现与完成标记](../../chapters/20_present_sync/README.md)

### 本课的真实源码切片

来自 [DirectX12/lessons/dx48_fence_lifecycle/main.cpp](main.cpp#L34)，第 34—50 行；这是现有实现的局部摘录，末尾可能处于函数中间，不是可单独编译的完整程序。

```cpp
int runChapter(int argc, char** argv) {
    auto options = labOptions(argc, argv, 48);
    DxDevice c(options.warp);
    {
        inspection(c, options);
    }
    c.checkMessages();
    return 0;
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
| DirectX 12 | [dx48_fence_lifecycle](README.md) |
| 对照后端 | [vk48_fence_lifecycle](../../../Vulkan/native/lessons/vk48_fence_lifecycle/README.md) |

### 先预测，再验证

解释为何 Vulkan 在 reset 后遇到 OUT_OF_DATE 并直接 continue，可能让下一帧永久等待。

参考推导：该轮没有提交去 signal 已重置 fence。正确顺序应先处理 acquire 的可恢复失败，再对确定会提交的路径重置。

## 验证与边界

运行 `--smoke` 执行有限自动检查。数值实验读取实际 GPU 结果，图形实验检查非空图像并可查看读回。交互重建与性能还需额外证据；窗口 renderer 当前保留串行等待，双槽计算实验不会被描述为完整多帧呈现引擎。
