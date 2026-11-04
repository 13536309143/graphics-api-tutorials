# vk48_fence_lifecycle · 完成标记生命周期

[54 课目录](../../COURSE.md)

## 目的与实际执行

acquire semaphore 让提交等待图像可用，render-finished semaphore 供 present 等待，fence 让 CPU 复用上一代资源。独立实验验证 signaled→reset→submit→complete；实际窗口路径只在成功 acquire 后 reset fence，避免错误分支留下永远不会完成的等待。

本程序有独立实验选择与验证路径，公共实现集中在 advanced_labs.cpp。设备/API 检查、数值 compute 与图形渲染采用不同证据；不是每个程序都会弹出窗口。

## 构建和运行

```powershell
cmake --preset vulkan-native
cmake --build --preset vulkan-native-debug --target vk48_fence_lifecycle --parallel 4
.\Vulkan\native\build\bin\Debug\vk48_fence_lifecycle.exe
```

## 深入学习：数据、接口与执行

- [第 20 课：获取、提交、呈现与完成标记](../../chapters/20_present_sync/README.md)

### 本课的真实源码切片

来自 [Vulkan/native/common/advanced_labs.cpp](../../common/advanced_labs.cpp#L890)，第 890—933 行；这是现有实现的局部摘录，末尾可能处于函数中间，不是可单独编译的完整程序。

```cpp
        require(r.size >= 257 && r.alignment > 0, "Invalid memory requirements");
        std::cout << "requested=257, allocation=" << r.size << ", alignment=" << r.alignment
                  << ", memoryTypeBits=" << r.memoryTypeBits << '\n';
    } else if (id == 48) {
        VkFenceCreateInfo fi{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
        fi.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        VkFence fence{};
        vkCheck(vkCreateFence(c.device, &fi, nullptr, &fence));
        require(vkGetFenceStatus(c.device, fence) == VK_SUCCESS, "Initial fence not signaled");
        vkCheck(vkResetFences(c.device, 1, &fence));
        require(vkGetFenceStatus(c.device, fence) == VK_NOT_READY,
                "Reset fence signaled unexpectedly");
        VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};
        vkCheck(vkQueueSubmit(c.queue, 1, &submit, fence));
        vkCheck(vkWaitForFences(c.device, 1, &fence, VK_TRUE, UINT64_MAX));
        vkDestroyFence(c.device, fence, nullptr);
        std::cout << "Verified signaled -> reset -> submitted -> complete fence lifecycle\n";
    } else if (id == 51) {
        static_assert(sizeof(Frame) == 64 && sizeof(Draw) == 80 && offsetof(Draw, settings) == 64);
        ComputeKernel kernel(c);
        kernel.execute(1, 128, 0, 0, 2);
        auto r = readWords(kernel.output, 128);
        for (uint32_t i = 0; i < 128; ++i)
            require(r[i] == i * 7 + 13, "UBO layout mismatch");
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
