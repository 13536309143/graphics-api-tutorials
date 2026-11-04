# dx54_acceptance_suite · 资源正确性综合验收

[54 课目录](../../COURSE.md)

## 目的与实际执行

同样的 CPU 几何进入独立后端，texture view、MSAA resolve 和 CBV/PSO 使用期由 DX12 规则维护。CPU 生成正确 sRGB mip 与线性 GPU compute mip 是两条明确路径，不能混称同一种算法。

本程序有独立实验选择与验证路径，公共实现集中在 advanced_labs.cpp。设备/API 检查、数值 compute 与图形渲染采用不同证据；不是每个程序都会弹出窗口。

## 构建和运行

```powershell
cmake --preset dx12
cmake --build --preset dx12-debug --target dx54_acceptance_suite --parallel 4
.\DirectX12\build\bin\Debug\dx54_acceptance_suite.exe
```

## 深入学习：数据、接口与执行

- [第 54 课：综合场景、验收与工程化边界](../../chapters/54_capstone/README.md)

### 本课的真实源码切片

来自 [DirectX12/common/advanced_labs.cpp](../../common/advanced_labs.cpp#L734)，第 734—777 行；这是现有实现的局部摘录，末尾可能处于函数中间，不是可单独编译的完整程序。

```cpp
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
    } else if (id == 13)
        stagingCopy(c);
    else if (id == 18) {
        GpuTexture texture(c, options, Pixels{64, 64, checker()}, true);
        texture.verify();
    } else if (id == 24)
        frameRing(c, options);
```

### 同主题跨 API 导航

| 路线 | 程序 |
| --- | --- |
| DirectX 12 | [dx54_acceptance_suite](README.md) |
| 对照后端 | [vk54_acceptance_suite](../../../Vulkan/native/lessons/vk54_acceptance_suite/README.md) |

### 先预测，再验证

选择阴影、IBL 或骨骼动画之一，先写资源表、pass 依赖图、中间调试视图和验收条件，再实现。

参考推导：原生/DX12 当前完成静态模型、mip/MSAA、直接光 PBR 和计算扩展；完整 glTF 材质、skin/动画、IBL、SSAO 等仍需单独工程实现与验证。清楚标记边界比扩大功能名列表更有价值。

## 验证与边界

运行 `--smoke` 执行有限自动检查。数值实验读取实际 GPU 结果，图形实验检查非空图像并可查看读回。交互重建与性能还需额外证据；窗口 renderer 当前保留串行等待，双槽计算实验不会被描述为完整多帧呈现引擎。
