# vk45_descriptor_update · 资源引用替换

[54 课目录](../../COURSE.md)

## 目的与实际执行

pool 按描述符类型与数量提供分配容量，set 从 pool 分配。update 替换 binding 0 的 storage buffer，必须等原使用完成并遵循更新规则；普通 descriptor set 不能在 GPU 使用期间随意改写。

本程序有独立实验选择与验证路径，公共实现集中在 advanced_labs.cpp。设备/API 检查、数值 compute 与图形渲染采用不同证据；不是每个程序都会弹出窗口。

## 构建和运行

```powershell
cmake --preset vulkan-native
cmake --build --preset vulkan-native-debug --target vk45_descriptor_update --parallel 4
.\Vulkan\native\build\bin\Debug\vk45_descriptor_update.exe
```

## 深入学习：数据、接口与执行

- [第 30 课：描述符池、Heap 与资源引用更新](../../chapters/30_descriptor_instances/README.md)

### 本课的真实源码切片

来自 [Vulkan/native/common/advanced_labs.cpp](../../common/advanced_labs.cpp#L867)，第 867—910 行；这是现有实现的局部摘录，末尾可能处于函数中间，不是可单独编译的完整程序。

```cpp
                             VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                             VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
                             VK_PIPELINE_STAGE_2_HOST_BIT,
                             VK_ACCESS_2_HOST_READ_BIT);
        });
        auto changed = readWords(second, 128);
        require(readWords(kernel.output, 128) == first, "Descriptor update overwrote prior buffer");
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
```

### 同主题跨 API 导航

| 路线 | 程序 |
| --- | --- |
| Vulkan 原生 | [vk45_descriptor_update](README.md) |
| 对照后端 | [dx45_descriptor_update](../../../../DirectX12/lessons/dx45_descriptor_update/README.md) |

### 先预测，再验证

修改绑定到 B 后，为什么还要检查 A？

参考推导：这能发现 CPU 误写旧资源、错误绑定或读回来源混淆。只看 B 的某个值可能被旧内容偶然满足。

## 验证与边界

运行 `--smoke` 执行有限自动检查。数值实验读取实际 GPU 结果，图形实验检查非空图像并可查看读回。交互重建与性能还需额外证据；窗口 renderer 当前保留串行等待，双槽计算实验不会被描述为完整多帧呈现引擎。
