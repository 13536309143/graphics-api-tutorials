# vk46_depth_formats · 深度格式支持

[54 课目录](../../COURSE.md)

## 目的与实际执行

查询 D32、D24S8、D16 的 optimal tiling features，检查 DEPTH_STENCIL_ATTACHMENT。view 的 aspect 要与实际使用一致；若包含 stencil，不能无条件只按纯 depth 格式处理所有布局与 view。

本程序有独立实验选择与验证路径，公共实现集中在 advanced_labs.cpp。设备/API 检查、数值 compute 与图形渲染采用不同证据；不是每个程序都会弹出窗口。

## 构建和运行

```powershell
cmake --preset vulkan-native
cmake --build --preset vulkan-native-debug --target vk46_depth_formats --parallel 4
.\Vulkan\native\build\bin\Debug\vk46_depth_formats.exe
```

## 深入学习：数据、接口与执行

- [第 39 课：深度格式、用途与样本支持](../../chapters/39_depth_formats/README.md)

### 本课的真实源码切片

来自 [Vulkan/native/common/advanced_labs.cpp](../../common/advanced_labs.cpp#L874)，第 874—917 行；这是现有实现的局部摘录，末尾可能处于函数中间，不是可单独编译的完整程序。

```cpp
        for (uint32_t i = 0; i < 128; ++i)
            require(changed[i] == i * 5 + 11, "Updated descriptor points at wrong storage");
        std::cout << "Verified descriptor replacement after prior submission completion\n";
    } else if (id == 46) {
        for (auto format :
             {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D16_UNORM}) {
            VkFormatProperties p{};
            vkGetPhysicalDeviceFormatProperties(c.physical, format, &p);
            std::cout << "depth format=" << format
                      << ", optimal features=" << p.optimalTilingFeatures << '\n';
        }
    } else if (id == 47) {
        VkBufferResource buffer;
        buffer.init(c, 257, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
        VkMemoryRequirements r{};
        vkGetBufferMemoryRequirements(c.device, buffer.buffer, &r);
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
```

### 同主题跨 API 导航

| 路线 | 程序 |
| --- | --- |
| Vulkan 原生 | [vk46_depth_formats](README.md) |
| 对照后端 | [dx46_depth_formats](../../../../DirectX12/lessons/dx46_depth_formats/README.md) |

### 先预测，再验证

为颜色和深度选择共同支持的样本数，写出如何取合法交集。

参考推导：Vulkan 检查颜色/深度 sample flags 的交集并结合实际格式；DX12 分别查询对应格式的指定样本数，只有两者支持才能组合。

## 验证与边界

运行 `--smoke` 执行有限自动检查。数值实验读取实际 GPU 结果，图形实验检查非空图像并可查看读回。交互重建与性能还需额外证据；窗口 renderer 当前保留串行等待，双槽计算实验不会被描述为完整多帧呈现引擎。
