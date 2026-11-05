# dx51_uniform_layout · 常量字节布局

[54 课目录](../../COURSE.md)

## 本课独立工程与关键注释

[main.cpp](main.cpp) · [本地辅助实现](support/) · [本课着色器](shaders/) · [CMakeLists.txt](CMakeLists.txt)

本目录包含独立课程源码，关键位置有中文注释。单独复制整个文件夹后可用 `cmake -S . -B build -A x64` 配置、`cmake --build build --config Debug` 构建；不需要其他章节源码。

## 目的与实际执行

HLSL column_major 与 mul(M,v) 配合 GLM。root CBV 的 GPU 地址需要 256 字节对齐，实际帧矩阵仍只有 64 字节；根常量按 DWORD 更新。不要把所有 C++ struct 都假定与 cbuffer 打包自动一致。

本程序的 C++、辅助实现、着色器和构建脚本均位于本目录，关键步骤含中文注释。设备/API 检查、数值 compute 与图形渲染采用不同证据；不是每个程序都会弹出窗口。

## 构建和运行

```powershell
cmake --preset dx12
cmake --build --preset dx12-debug --target dx51_uniform_layout --parallel 4
.\DirectX12\build\bin\Debug\dx51_uniform_layout.exe
```

## 深入学习：数据、接口与执行

- [第 28 课：常量结构、矩阵存储与对齐](../../chapters/28_uniform_layout/README.md)

### 本课的真实源码切片

来自 [DirectX12/lessons/dx51_uniform_layout/main.cpp](main.cpp#L147)，第 147—163 行；这是现有实现的局部摘录，末尾可能处于函数中间，不是可单独编译的完整程序。

```cpp
int runChapter(int argc, char** argv) {
    auto options = labOptions(argc, argv, 51);
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
| DirectX 12 | [dx51_uniform_layout](README.md) |
| 对照后端 | [vk51_uniform_layout](../../../Vulkan/native/lessons/vk51_uniform_layout/README.md) |

### 先预测，再验证

在 CPU 打印 sizeof/offsetof，再通过 GPU 计算验证第三个常量槽的 factor/bias。

参考推导：CPU 布局断言检查内存，GPU 结果检查接口选择与解码；两份证据互补。第三槽得到 i*7+13，才说明该范围被正确使用。

## 验证与边界

运行 `--smoke` 执行有限自动检查。数值实验读取实际 GPU 结果，图形实验检查非空图像并可查看读回。交互重建与性能还需额外证据；窗口 renderer 当前保留串行等待，双槽计算实验不会被描述为完整多帧呈现引擎。
