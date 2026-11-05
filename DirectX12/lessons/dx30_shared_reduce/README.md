# dx30_shared_reduce · 组共享内存归约

[54 课目录](../../COURSE.md)

## 本课独立工程与关键注释

[main.cpp](main.cpp) · [本地辅助实现](support/) · [本课着色器](shaders/) · [CMakeLists.txt](CMakeLists.txt)

本目录包含独立课程源码，关键位置有中文注释。单独复制整个文件夹后可用 `cmake -S . -B build -A x64` 配置、`cmake --build build --config Debug` 构建；不需要其他章节源码。

## 目的与实际执行

groupshared 与 GroupMemoryBarrierWithGroupSync 承担相同算法职责。线程在屏障前非一致 return 可能造成未定义结果或卡住，因此边界检查只改变贡献值，不让部分 lane 跳过同步。

本程序的 C++、辅助实现、着色器和构建脚本均位于本目录，关键步骤含中文注释。设备/API 检查、数值 compute 与图形渲染采用不同证据；不是每个程序都会弹出窗口。

## 构建和运行

```powershell
cmake --preset dx12
cmake --build --preset dx12-debug --target dx30_shared_reduce --parallel 4
.\DirectX12\build\bin\Debug\dx30_shared_reduce.exe
```

## 深入学习：数据、接口与执行

- [第 49 课：组共享内存、归约与屏障一致性](../../chapters/49_group_shared/README.md)

### 本课的真实源码切片

来自 [DirectX12/lessons/dx30_shared_reduce/main.cpp](main.cpp#L136)，第 136—172 行；这是现有实现的局部摘录，末尾可能处于函数中间，不是可单独编译的完整程序。

```cpp
int runChapter(int argc, char** argv) {
    auto options = labOptions(argc, argv, 30);
    DxDevice c(options.warp);
    {
        ComputeKernel kernel(c, options);
        uint32_t mode = (2);
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
        std::cout << "Verified experiment " << 30 << ", items=" << count
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
| DirectX 12 | [dx30_shared_reduce](README.md) |
| 对照后端 | [vk30_shared_reduce](../../../Vulkan/native/lessons/vk30_shared_reduce/README.md) |

### 先预测，再验证

若尾部 lane 在第一个 barrier 前直接 return，为什么不是一种安全优化？

参考推导：组中部分线程仍在等待完整同步，屏障的控制流条件不再成立。应让它们贡献零并继续参与屏障。

## 验证与边界

运行 `--smoke` 执行有限自动检查。数值实验读取实际 GPU 结果，图形实验检查非空图像并可查看读回。交互重建与性能还需额外证据；窗口 renderer 当前保留串行等待，双槽计算实验不会被描述为完整多帧呈现引擎。
