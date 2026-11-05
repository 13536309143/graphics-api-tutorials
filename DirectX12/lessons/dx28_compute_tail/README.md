# dx28_compute_tail · 非整组尾部计算

[54 课目录](../../COURSE.md)

## 本课独立工程与关键注释

[main.cpp](main.cpp) · [本地辅助实现](support/) · [本课着色器](shaders/) · [CMakeLists.txt](CMakeLists.txt)

本目录包含独立课程源码，关键位置有中文注释。单独复制整个文件夹后可用 `cmake -S . -B build -A x64` 配置、`cmake --build build --config Debug` 构建；不需要其他章节源码。

## 目的与实际执行

RWStructuredBuffer<uint> 的结构元素大小与资源容量匹配；root UAV 选择地址但不会携带自动上界保护。即使资源足够大，逻辑上未写的元素也不能当成正确结果。

本程序的 C++、辅助实现、着色器和构建脚本均位于本目录，关键步骤含中文注释。设备/API 检查、数值 compute 与图形渲染采用不同证据；不是每个程序都会弹出窗口。

## 构建和运行

```powershell
cmake --preset dx12
cmake --build --preset dx12-debug --target dx28_compute_tail --parallel 4
.\DirectX12\build\bin\Debug\dx28_compute_tail.exe
```

## 深入学习：数据、接口与执行

- [第 47 课：Storage/UAV、边界与 257 项实验](../../chapters/47_storage_tail/README.md)

### 本课的真实源码切片

来自 [DirectX12/lessons/dx28_compute_tail/main.cpp](main.cpp#L136)，第 136—172 行；这是现有实现的局部摘录，末尾可能处于函数中间，不是可单独编译的完整程序。

```cpp
int runChapter(int argc, char** argv) {
    auto options = labOptions(argc, argv, 28);
    DxDevice c(options.warp);
    {
        ComputeKernel kernel(c, options);
        uint32_t mode = ((0));
        uint32_t count = (257);
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
        std::cout << "Verified experiment " << 28 << ", items=" << count
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
| DirectX 12 | [dx28_compute_tail](README.md) |
| 对照后端 | [vk28_compute_tail](../../../Vulkan/native/lessons/vk28_compute_tail/README.md) |

### 先预测，再验证

只把 CPU 验证改成 257 而仍分配 256 项，会产生什么问题？

参考推导：可能读取越界；只扩容量却不扩组数则尾项未写。正确验收不能用忽略最后一项来让测试通过。

## 验证与边界

运行 `--smoke` 执行有限自动检查。数值实验读取实际 GPU 结果，图形实验检查非空图像并可查看读回。交互重建与性能还需额外证据；窗口 renderer 当前保留串行等待，双槽计算实验不会被描述为完整多帧呈现引擎。
