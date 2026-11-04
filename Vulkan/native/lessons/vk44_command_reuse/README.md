# vk44_command_reuse · 命令存储安全重用

[54 课目录](../../COURSE.md)

## 目的与实际执行

从与队列族匹配的 pool 分配 command buffer，完成后 reset pool，再录制新一代 fill。pool reset 与单独 buffer reset 的 flag/生命周期规则不同；本实验选择整池重置以展示最清楚的拥有关系。

本程序有独立实验选择与验证路径，公共实现集中在 advanced_labs.cpp。设备/API 检查、数值 compute 与图形渲染采用不同证据；不是每个程序都会弹出窗口。

## 构建和运行

```powershell
cmake --preset vulkan-native
cmake --build --preset vulkan-native-debug --target vk44_command_reuse --parallel 4
.\Vulkan\native\build\bin\Debug\vk44_command_reuse.exe
```

## 深入学习：数据、接口与执行

- [第 18 课：命令池、Allocator 与安全重置](../../chapters/18_command_storage/README.md)

### 本课的真实源码切片

来自 [Vulkan/native/common/advanced_labs.cpp](../../common/advanced_labs.cpp#L726)，第 726—769 行；这是现有实现的局部摘录，末尾可能处于函数中间，不是可单独编译的完整程序。

```cpp
        std::cout << "Surface minImages=" << caps.minImageCount
                  << ", maxImages=" << caps.maxImageCount << ", formats=" << formats
                  << ", presentModes=" << modes << '\n';
    }
    glfwDestroyWindow(window);
    glfwTerminate();
}
// 整池重置必须发生在该代命令完成之后；每代写不同数据以发现旧命令残留。
void commandReuse(const VkContextRef& c) {
    VkBufferResource output;
    output.init(c, 256, VK_BUFFER_USAGE_TRANSFER_DST_BIT);
    VkCommandPoolCreateInfo pi{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    pi.queueFamilyIndex = c.family;
    VkCommandPool pool{};
    vkCheck(vkCreateCommandPool(c.device, &pi, nullptr, &pool));
    VkCommandBufferAllocateInfo ai{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    ai.commandPool = pool;
    ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    ai.commandBufferCount = 1;
    VkCommandBuffer cmd{};
    vkCheck(vkAllocateCommandBuffers(c.device, &ai, &cmd));
    VkFenceCreateInfo fi{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    VkFence fence{};
    vkCheck(vkCreateFence(c.device, &fi, nullptr, &fence));
    for (uint32_t generation = 0; generation < 4; ++generation) {
        vkCheck(vkResetCommandPool(c.device, pool, 0));
        VkCommandBufferBeginInfo bi{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
        vkCheck(vkBeginCommandBuffer(cmd, &bi));
        vkCmdFillBuffer(cmd, output.buffer, 0, 256, 41 + generation);
        bufferDependency(cmd,
                         output.buffer,
                         VK_PIPELINE_STAGE_2_CLEAR_BIT,
                         VK_ACCESS_2_TRANSFER_WRITE_BIT,
                         VK_PIPELINE_STAGE_2_HOST_BIT,
                         VK_ACCESS_2_HOST_READ_BIT);
        vkCheck(vkEndCommandBuffer(cmd));
        VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};
        submit.commandBufferCount = 1;
        submit.pCommandBuffers = &cmd;
        vkCheck(vkQueueSubmit(c.queue, 1, &submit, fence));
        vkCheck(vkWaitForFences(c.device, 1, &fence, VK_TRUE, UINT64_MAX));
        for (auto x : readWords(output, 64))
            require(x == 41 + generation, "Reset command storage contains stale data");
        vkCheck(vkResetFences(c.device, 1, &fence));
```

### 同主题跨 API 导航

| 路线 | 程序 |
| --- | --- |
| Vulkan 原生 | [vk44_command_reuse](README.md) |
| 对照后端 | [dx44_command_reuse](../../../../DirectX12/lessons/dx44_command_reuse/README.md) |

### 先预测，再验证

四次录制分别写 41、42、43、44，怎样验证重置与执行链路？

参考推导：每代都等待其完成并核对全部结果，再允许下一次覆盖命令存储。仅最终值正确不能证明中间代次没有生命周期错误。

## 验证与边界

运行 `--smoke` 执行有限自动检查。数值实验读取实际 GPU 结果，图形实验检查非空图像并可查看读回。交互重建与性能还需额外证据；窗口 renderer 当前保留串行等待，双槽计算实验不会被描述为完整多帧呈现引擎。
