# dx48_fence_lifecycle · 完成标记生命周期

[54 课目录](../../COURSE.md)

## 目的与实际执行

back buffer 在 PRESENT/COMMON 与渲染用途间转换，Present 与 queue fence 各司其职。DX12 fence 是单调计数，不能套用 Vulkan binary fence 的 reset 模型；实验连续推进完成值并检查。

本程序有独立实验选择与验证路径，公共实现集中在 advanced_labs.cpp。设备/API 检查、数值 compute 与图形渲染采用不同证据；不是每个程序都会弹出窗口。

## 构建和运行

```powershell
cmake --preset dx12
cmake --build --preset dx12-debug --target dx48_fence_lifecycle --parallel 4
.\DirectX12\build\bin\Debug\dx48_fence_lifecycle.exe
```

## 深入学习：数据、接口与执行

- [第 20 课：获取、提交、呈现与完成标记](../../chapters/20_present_sync/README.md)

### 本课的真实源码切片

来自 [DirectX12/common/advanced_labs.cpp](../../common/advanced_labs.cpp#L718)，第 718—761 行；这是现有实现的局部摘录，末尾可能处于函数中间，不是可单独编译的完整程序。

```cpp
                  << ", CPUPageProperty=" << upload.CPUPageProperty << '\n';
    } else if (id == 48) {
        auto initial = c.fence->GetCompletedValue();
        dxCheck(c.queue->Signal(c.fence.Get(), ++c.value));
        waitValue(c, c.value);
        require(c.fence->GetCompletedValue() > initial, "Fence counter failed to advance");
        dxCheck(c.queue->Signal(c.fence.Get(), ++c.value));
        waitValue(c, c.value);
        std::cout << "Verified monotonic fence reuse; DX12 fence is not reset like a binary Vulkan "
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
