# vk24_frame_ring · 双帧计算资源环

[54 课目录](../../COURSE.md)

## 目的与实际执行

每槽有独立 ComputeKernel 输出、command buffer 与 fence，slot=frameNumber%2。先等该槽再 reset/更新，GPU 仍可执行另一个槽的工作；最终检查每槽最后一代结果。

本程序有独立实验选择与验证路径，公共实现集中在 advanced_labs.cpp。设备/API 检查、数值 compute 与图形渲染采用不同证据；不是每个程序都会弹出窗口。

## 构建和运行

```powershell
cmake --preset vulkan-native
cmake --build --preset vulkan-native-debug --target vk24_frame_ring --parallel 4
.\Vulkan\native\build\bin\Debug\vk24_frame_ring.exe
```

## 深入学习：数据、接口与执行

- [第 21 课：两帧资源环与在途使用期](../../chapters/21_frames_in_flight/README.md)

### 本课的真实源码切片

来自 [Vulkan/native/common/advanced_labs.cpp](../../common/advanced_labs.cpp#L200)，第 200—243 行；这是现有实现的局部摘录，末尾可能处于函数中间，不是可单独编译的完整程序。

```cpp
void frameRing(const VkContextRef& c) {
    // Two independent storage buffers and command buffers remain in flight.
    // This isolates frame-slot lifetime; windowed renderers still use the simple serialized path.
    ComputeKernel kernels[2] = {ComputeKernel(c), ComputeKernel(c)};
    VkCommandPoolCreateInfo pi{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    pi.queueFamilyIndex = c.family;
    pi.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    VkCommandPool pool{};
    vkCheck(vkCreateCommandPool(c.device, &pi, nullptr, &pool));
    VkCommandBufferAllocateInfo ai{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    ai.commandPool = pool;
    ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    ai.commandBufferCount = 2;
    VkCommandBuffer commands[2]{};
    vkCheck(vkAllocateCommandBuffers(c.device, &ai, commands));
    VkFence fences[2]{};
    VkFenceCreateInfo fi{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    fi.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    for (auto& fence : fences)
        vkCheck(vkCreateFence(c.device, &fi, nullptr, &fence));
    int previous[2] = {-1, -1};
    for (uint32_t frameNumber = 0; frameNumber < 8; ++frameNumber) {
        uint32_t slot = frameNumber % 2;
        vkCheck(vkWaitForFences(c.device, 1, &fences[slot], VK_TRUE, UINT64_MAX));
        if (previous[slot] >= 0) {
            auto result = readWords(kernels[slot].output, 257);
            for (uint32_t i = 0; i < 257; ++i)
                require(result[i] == i * 3 + uint32_t(previous[slot]),
                        "In-flight slot overwritten");
        }
        vkCheck(vkResetFences(c.device, 1, &fences[slot]));
        vkCheck(vkResetCommandBuffer(commands[slot], 0));
        VkCommandBufferBeginInfo bi{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
        vkCheck(vkBeginCommandBuffer(commands[slot], &bi));
        kernels[slot].record(commands[slot], 0, 257, 3, frameNumber);
        vkCheck(vkEndCommandBuffer(commands[slot]));
        VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};
        submit.commandBufferCount = 1;
        submit.pCommandBuffers = &commands[slot];
        vkCheck(vkQueueSubmit(c.queue, 1, &submit, fences[slot]));
        previous[slot] = int(frameNumber);
    }
    vkCheck(vkWaitForFences(c.device, 2, fences, VK_TRUE, UINT64_MAX));
    for (uint32_t slot = 0; slot < 2; ++slot) {
```

### 同主题跨 API 导航

| 路线 | 程序 |
| --- | --- |
| Vulkan 原生 | [vk24_frame_ring](README.md) |
| 对照后端 | [dx24_frame_ring](../../../../DirectX12/lessons/dx24_frame_ring/README.md) |

### 先预测，再验证

把槽数改为三，列出所有必须一起改变的数组和完成条件。

参考推导：命令存储、可变缓冲、每槽记录与最终等待均需对应三份。交换链图像数量不必因此变三，imageIndex 仍来自呈现系统。

## 验证与边界

运行 `--smoke` 执行有限自动检查。数值实验读取实际 GPU 结果，图形实验检查非空图像并可查看读回。交互重建与性能还需额外证据；窗口 renderer 当前保留串行等待，双槽计算实验不会被描述为完整多帧呈现引擎。
