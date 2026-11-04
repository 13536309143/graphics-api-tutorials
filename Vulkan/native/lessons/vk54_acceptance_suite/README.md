# vk54_acceptance_suite · 资源正确性综合验收

[54 课目录](../../COURSE.md)

## 目的与实际执行

综合例通过 SceneVk 的显式配置连接资产、纹理与目标，保持可追踪资源拥有者。resize 重建时配置回调重建所需引用。当前窗口仍按帧等待，计算资源环证明两槽原则而非替代完整多帧呈现引擎。

本程序有独立实验选择与验证路径，公共实现集中在 advanced_labs.cpp。设备/API 检查、数值 compute 与图形渲染采用不同证据；不是每个程序都会弹出窗口。

## 构建和运行

```powershell
cmake --preset vulkan-native
cmake --build --preset vulkan-native-debug --target vk54_acceptance_suite --parallel 4
.\Vulkan\native\build\bin\Debug\vk54_acceptance_suite.exe
```

## 深入学习：数据、接口与执行

- [第 54 课：综合场景、验收与工程化边界](../../chapters/54_capstone/README.md)

### 本课的真实源码切片

来自 [Vulkan/native/common/advanced_labs.cpp](../../common/advanced_labs.cpp#L914)，第 914—957 行；这是现有实现的局部摘录，末尾可能处于函数中间，不是可单独编译的完整程序。

```cpp
        std::cout
            << "Verified Frame=64, Draw=80, settings offset=64 and GPU uniform vector decoding\n";
    } else if (id == 54) {
        capabilities(c);
        stagingCopy(c);
        GpuTexture mips(c, Pixels{64, 64, checker()}, true);
        mips.verify();
        frameRing(c);
        std::cout << "Completed resource/texture/frame-slot acceptance suite\n";
    } else
        throw std::runtime_error("Unknown inspection id");
}

} // namespace
int nativeLabMain(int argc, char** argv, int id) {
    auto options = labOptions(argc, argv, id);
    nativeShaders = options.directory / "shaders";
    if (id == 17 || id == 19 || id == 20 || id == 21 || id == 22 || id == 23 || id == 27 ||
        id == 29 || id == 34 || id == 35 || id == 36 || id == 41 || id == 43 || id == 49 ||
        id == 50 || id == 52 || id == 53)
        return graphicsLab(argc, argv, options);
    validationErrors = 0;
    if (id == 40) {
        surfaceInspection();
        return validationErrors ? 1 : 0;
    }
    {
        NativeDevice owner(nullptr, id == 26);
        const auto& c = owner.c;
        if (id >= 37)
            inspection(c, id);
        else if (id == 11)
            capabilities(c);
        else if (id == 12) {
            VkBufferResource b;
            b.init(c, 256, VK_BUFFER_USAGE_TRANSFER_DST_BIT);
            immediate(c, [&](VkCommandBuffer cmd) {
                vkCmdFillBuffer(cmd, b.buffer, 0, 256, 0x12345678);
                bufferDependency(cmd,
                                 b.buffer,
                                 VK_PIPELINE_STAGE_2_COPY_BIT | VK_PIPELINE_STAGE_2_CLEAR_BIT,
                                 VK_ACCESS_2_TRANSFER_WRITE_BIT,
                                 VK_PIPELINE_STAGE_2_HOST_BIT,
                                 VK_ACCESS_2_HOST_READ_BIT);
```

### 同主题跨 API 导航

| 路线 | 程序 |
| --- | --- |
| Vulkan 原生 | [vk54_acceptance_suite](README.md) |
| 对照后端 | [dx54_acceptance_suite](../../../../DirectX12/lessons/dx54_acceptance_suite/README.md) |

### 先预测，再验证

选择阴影、IBL 或骨骼动画之一，先写资源表、pass 依赖图、中间调试视图和验收条件，再实现。

参考推导：原生/DX12 当前完成静态模型、mip/MSAA、直接光 PBR 和计算扩展；完整 glTF 材质、skin/动画、IBL、SSAO 等仍需单独工程实现与验证。清楚标记边界比扩大功能名列表更有价值。

## 验证与边界

运行 `--smoke` 执行有限自动检查。数值实验读取实际 GPU 结果，图形实验检查非空图像并可查看读回。交互重建与性能还需额外证据；窗口 renderer 当前保留串行等待，双槽计算实验不会被描述为完整多帧呈现引擎。
