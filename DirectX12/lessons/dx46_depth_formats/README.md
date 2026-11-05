# dx46_depth_formats · 深度格式支持

[54 课目录](../../COURSE.md)

## 本课独立工程与关键注释

[main.cpp](main.cpp) · [本地辅助实现](support/) · [本课着色器](shaders/) · [CMakeLists.txt](CMakeLists.txt)

本目录包含独立课程源码，关键位置有中文注释。单独复制整个文件夹后可用 `cmake -S . -B build -A x64` 配置、`cmake --build build --config Debug` 构建；不需要其他章节源码。

## 目的与实际执行

FORMAT_SUPPORT 的 depth-stencil 支持与 MULTISAMPLE_QUALITY_LEVELS 分别查询。DSV 的格式/维度和 PSO 的 DSVFormat、sample count 要一致，resource flags 还需允许深度用途。

本程序的 C++、辅助实现、着色器和构建脚本均位于本目录，关键步骤含中文注释。设备/API 检查、数值 compute 与图形渲染采用不同证据；不是每个程序都会弹出窗口。

## 构建和运行

```powershell
cmake --preset dx12
cmake --build --preset dx12-debug --target dx46_depth_formats --parallel 4
.\DirectX12\build\bin\Debug\dx46_depth_formats.exe
```

## 深入学习：数据、接口与执行

- [第 39 课：深度格式、用途与样本支持](../../chapters/39_depth_formats/README.md)

### 本课的真实源码切片

来自 [DirectX12/lessons/dx46_depth_formats/main.cpp](main.cpp#L24)，第 24—40 行；这是现有实现的局部摘录，末尾可能处于函数中间，不是可单独编译的完整程序。

```cpp
int runChapter(int argc, char** argv) {
    auto options = labOptions(argc, argv, 46);
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
| DirectX 12 | [dx46_depth_formats](README.md) |
| 对照后端 | [vk46_depth_formats](../../../Vulkan/native/lessons/vk46_depth_formats/README.md) |

### 先预测，再验证

为颜色和深度选择共同支持的样本数，写出如何取合法交集。

参考推导：Vulkan 检查颜色/深度 sample flags 的交集并结合实际格式；DX12 分别查询对应格式的指定样本数，只有两者支持才能组合。

## 验证与边界

运行 `--smoke` 执行有限自动检查。数值实验读取实际 GPU 结果，图形实验检查非空图像并可查看读回。交互重建与性能还需额外证据；窗口 renderer 当前保留串行等待，双槽计算实验不会被描述为完整多帧呈现引擎。
