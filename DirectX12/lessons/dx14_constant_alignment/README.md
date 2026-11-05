# dx14_constant_alignment · 常量缓冲对齐

[54 课目录](../../COURSE.md)

## 本课独立工程与关键注释

[main.cpp](main.cpp) · [本地辅助实现](support/) · [本课着色器](shaders/) · [CMakeLists.txt](CMakeLists.txt)

本目录包含独立课程源码，关键位置有中文注释。单独复制整个文件夹后可用 `cmake -S . -B build -A x64` 配置、`cmake --build build --config Debug` 构建；不需要其他章节源码。

## 目的与实际执行

GetResourceAllocationInfo 解释实际分配大小/对齐，UPLOAD/DEFAULT/READBACK 表达常见用途。GetCustomHeapProperties 可以帮助理解架构映射，但不能随意绕过 heap/resource 规则。CBV 的 256 字节地址对齐与普通 buffer 分配对齐也不是同一要求。

HLSL column_major 与 mul(M,v) 配合 GLM。root CBV 的 GPU 地址需要 256 字节对齐，实际帧矩阵仍只有 64 字节；根常量按 DWORD 更新。不要把所有 C++ struct 都假定与 cbuffer 打包自动一致。

本程序的 C++、辅助实现、着色器和构建脚本均位于本目录，关键步骤含中文注释。设备/API 检查、数值 compute 与图形渲染采用不同证据；不是每个程序都会弹出窗口。

## 构建和运行

```powershell
cmake --preset dx12
cmake --build --preset dx12-debug --target dx14_constant_alignment --parallel 4
.\DirectX12\build\bin\Debug\dx14_constant_alignment.exe
```

## 深入学习：数据、接口与执行

- [第 25 课：内存类型、Heap 与分配要求](../../chapters/25_memory/README.md)
- [第 28 课：常量结构、矩阵存储与对齐](../../chapters/28_uniform_layout/README.md)

### 本课的真实源码切片

来自 [DirectX12/lessons/dx14_constant_alignment/main.cpp](main.cpp#L136)，第 136—172 行；这是现有实现的局部摘录，末尾可能处于函数中间，不是可单独编译的完整程序。

```cpp
int runChapter(int argc, char** argv) {
    auto options = labOptions(argc, argv, 14);
    DxDevice c(options.warp);
    {
        ComputeKernel kernel(c, options);
        uint32_t mode = ((1));
        uint32_t count = (128);
        for (uint32_t slot = 0; slot < ((1u)); ++slot) {
            uint32_t factor = (3), bias = (7);
            kernel.execute(mode, count, factor, bias, slot);
            auto result = readBuffer(kernel.readback.Get(), mode == 2 ? (count + 63) / 64 : count);
            if (mode == 2) {
                auto sum = std::accumulate(result.begin(), result.end(), 0u);
                require(sum == count * (count + 1) / 2, "Shared reduction result mismatch");
            } else {
                if (mode == 1) {
                    factor = slot == 0 ? 2 : slot == 1 ? 5 : 7;
                    bias = slot == 0 ? 3 : slot == 1 ? 11 : 13;
                }
                for (uint32_t i = 0; i < count; ++i)
                    require(result[i] == i * factor + bias, "Compute/root-binding mismatch");
            }
        }
        std::cout << "Verified experiment " << 14 << ", items=" << count
                  << ", constant stride=256\n";
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
| DirectX 12 | [dx14_constant_alignment](README.md) |
| 对照后端 | [vk14_constant_alignment](../../../Vulkan/native/lessons/vk14_constant_alignment/README.md) |

### 先预测，再验证

比较请求 257 字节、返回的 allocation size、常量槽 stride 三者，解释不同来源。

参考推导：257 是应用内容大小，allocation size 来自内存分配约束，常量 stride 来自接口偏移对齐。三者可以不同而同时正确。

在 CPU 打印 sizeof/offsetof，再通过 GPU 计算验证第三个常量槽的 factor/bias。

参考推导：CPU 布局断言检查内存，GPU 结果检查接口选择与解码；两份证据互补。第三槽得到 i*7+13，才说明该范围被正确使用。

## 验证与边界

运行 `--smoke` 执行有限自动检查。数值实验读取实际 GPU 结果，图形实验检查非空图像并可查看读回。交互重建与性能还需额外证据；窗口 renderer 当前保留串行等待，双槽计算实验不会被描述为完整多帧呈现引擎。
