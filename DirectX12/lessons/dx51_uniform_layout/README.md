# dx51_uniform_layout · 常量字节布局

[54 课目录](../../COURSE.md)

## 目的与实际执行

HLSL column_major 与 mul(M,v) 配合 GLM。root CBV 的 GPU 地址需要 256 字节对齐，实际帧矩阵仍只有 64 字节；根常量按 DWORD 更新。不要把所有 C++ struct 都假定与 cbuffer 打包自动一致。

本程序有独立实验选择与验证路径，公共实现集中在 advanced_labs.cpp。设备/API 检查、数值 compute 与图形渲染采用不同证据；不是每个程序都会弹出窗口。

## 构建和运行

```powershell
cmake --preset dx12
cmake --build --preset dx12-debug --target dx51_uniform_layout --parallel 4
.\DirectX12\build\bin\Debug\dx51_uniform_layout.exe
```

## 深入学习：数据、接口与执行

- [第 28 课：常量结构、矩阵存储与对齐](../../chapters/28_uniform_layout/README.md)

### 本课的真实源码切片

来自 [DirectX12/common/advanced_labs.cpp](../../common/advanced_labs.cpp#L727)，第 727—770 行；这是现有实现的局部摘录，末尾可能处于函数中间，不是可单独编译的完整程序。

```cpp
                     "fence\n";
    } else if (id == 51) {
        static_assert(sizeof(Frame) == 64 && sizeof(Draw) == 80 && offsetof(Draw, settings) == 64);
        ComputeKernel k(c, options);
        k.execute(1, 128, 0, 0, 2);
        auto r = readBuffer(k.readback.Get(), 128);
        for (uint32_t i = 0; i < 128; ++i)
            require(r[i] == i * 7 + 13, "CBV vector layout mismatch");
        std::cout << "Verified Frame=64, Draw=80, offset=64 and 256-byte-aligned CBV selection\n";
    } else if (id == 54) {
        capabilities(c);
        stagingCopy(c);
        GpuTexture mips(c, options, Pixels{64, 64, checker()}, true);
        mips.verify();
        frameRing(c, options);
        std::cout << "Completed buffer/texture/frame-slot acceptance suite\n";
    } else
        throw std::runtime_error("Unknown inspection id");
}

} // namespace
int dxLabMain(int argc, char** argv, int id) {
    auto options = labOptions(argc, argv, id);
    if (id == 17 || id == 19 || id == 20 || id == 21 || id == 22 || id == 23 || id == 27 ||
        id == 29 || id == 34 || id == 35 || id == 36 || id == 41 || id == 43 || id == 49 ||
        id == 50 || id == 52 || id == 53)
        return graphicsLab(argc, argv, options);
    if (id == 40) {
        surfaceInspection(options);
        return 0;
    }
    DxDevice c(options.warp);
    if (id >= 37)
        inspection(c, options, id);
    else if (id == 11)
        capabilities(c);
    else if (id == 12) {
        c.begin();
        dxCheck(c.cmd->Close());
        ID3D12CommandList* lists[] = {c.cmd.Get()};
        c.queue->ExecuteCommandLists(1, lists);
        c.wait();
        require(c.fence->GetCompletedValue() >= c.value, "Queue fence did not complete");
        std::cout << "Verified command list close/submit and fence completion\n";
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
