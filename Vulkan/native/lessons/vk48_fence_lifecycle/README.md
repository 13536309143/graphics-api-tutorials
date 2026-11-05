# vk48_fence_lifecycle · 完成标记生命周期

[54 课目录](../../COURSE.md)

## 本课独立工程与关键注释

[main.cpp](main.cpp) · [本地辅助实现](support/) · [本课着色器](shaders/) · [CMakeLists.txt](CMakeLists.txt)

本目录包含独立课程源码，关键位置有中文注释。单独复制整个文件夹后可用 `cmake -S . -B build -A x64` 配置、`cmake --build build --config Debug` 构建；不需要其他章节源码。

## 目的与实际执行

acquire semaphore 让提交等待图像可用，render-finished semaphore 供 present 等待，fence 让 CPU 复用上一代资源。独立实验验证 signaled→reset→submit→complete；实际窗口路径只在成功 acquire 后 reset fence，避免错误分支留下永远不会完成的等待。

本程序的 C++、辅助实现、着色器和构建脚本均位于本目录，关键步骤含中文注释。设备/API 检查、数值 compute 与图形渲染采用不同证据；不是每个程序都会弹出窗口。

## 构建和运行

```powershell
cmake --preset vulkan-native
cmake --build --preset vulkan-native-debug --target vk48_fence_lifecycle --parallel 4
.\Vulkan\native\build\bin\Debug\vk48_fence_lifecycle.exe
```

## 深入学习：数据、接口与执行

- [第 20 课：获取、提交、呈现与完成标记](../../chapters/20_present_sync/README.md)

### 本课的真实源码切片

来自 [Vulkan/native/lessons/vk48_fence_lifecycle/main.cpp](main.cpp#L29)，第 29—50 行；这是现有实现的局部摘录，末尾可能处于函数中间，不是可单独编译的完整程序。

```cpp
int runChapter(int argc, char** argv) {
    auto options = labOptions(argc, argv, 48);
    nativeShaders = options.directory / "shaders";
    validationErrors = 0;
    {
        NativeDevice owner(nullptr, false);
        const auto& c = owner.c;
        {
            inspection(c);
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
| Vulkan 原生 | [vk48_fence_lifecycle](README.md) |
| 对照后端 | [dx48_fence_lifecycle](../../../../DirectX12/lessons/dx48_fence_lifecycle/README.md) |

### 先预测，再验证

解释为何 Vulkan 在 reset 后遇到 OUT_OF_DATE 并直接 continue，可能让下一帧永久等待。

参考推导：该轮没有提交去 signal 已重置 fence。正确顺序应先处理 acquire 的可恢复失败，再对确定会提交的路径重置。

## 验证与边界

运行 `--smoke` 执行有限自动检查。数值实验读取实际 GPU 结果，图形实验检查非空图像并可查看读回。交互重建与性能还需额外证据；窗口 renderer 当前保留串行等待，双槽计算实验不会被描述为完整多帧呈现引擎。
