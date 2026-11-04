# vk26_timeline_counter · 计数同步与生产消费

[54 课目录](../../COURSE.md)

## 目的与实际执行

设备额外查询并启用 timelineSemaphore。第一次提交 signal=1，第二次 wait=1/signal=2，host 等到 2 并读取 counter。使用 submit2 的阶段范围明确，不把 timeline semaphore 当作交换链 binary semaphore 的直接替代。

本程序有独立实验选择与验证路径，公共实现集中在 advanced_labs.cpp。设备/API 检查、数值 compute 与图形渲染采用不同证据；不是每个程序都会弹出窗口。

## 构建和运行

```powershell
cmake --preset vulkan-native
cmake --build --preset vulkan-native-debug --target vk26_timeline_counter --parallel 4
.\Vulkan\native\build\bin\Debug\vk26_timeline_counter.exe
```

## 深入学习：数据、接口与执行

- [第 50 课：Timeline、Fence 计数与生产消费链](../../chapters/50_timeline/README.md)

### 本课的真实源码切片

来自 [Vulkan/native/common/advanced_labs.cpp](../../common/advanced_labs.cpp#L253)，第 253—296 行；这是现有实现的局部摘录，末尾可能处于函数中间，不是可单独编译的完整程序。

```cpp
void timelineCounter(const VkContextRef& c) {
    VkSemaphoreTypeCreateInfo type{VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO};
    type.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
    VkSemaphoreCreateInfo info{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    info.pNext = &type;
    VkSemaphore semaphore{};
    vkCheck(vkCreateSemaphore(c.device, &info, nullptr, &semaphore));
    VkSemaphoreSubmitInfo signal{VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO};
    signal.semaphore = semaphore;
    signal.value = 1;
    signal.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
    VkSubmitInfo2 first{VK_STRUCTURE_TYPE_SUBMIT_INFO_2};
    first.signalSemaphoreInfoCount = 1;
    first.pSignalSemaphoreInfos = &signal;
    vkCheck(vkQueueSubmit2(c.queue, 1, &first, VK_NULL_HANDLE));
    VkSemaphoreSubmitInfo wait = signal;
    signal.value = 2;
    VkSubmitInfo2 second{VK_STRUCTURE_TYPE_SUBMIT_INFO_2};
    second.waitSemaphoreInfoCount = 1;
    second.pWaitSemaphoreInfos = &wait;
    second.signalSemaphoreInfoCount = 1;
    second.pSignalSemaphoreInfos = &signal;
    vkCheck(vkQueueSubmit2(c.queue, 1, &second, VK_NULL_HANDLE));
    uint64_t target = 2;
    VkSemaphoreWaitInfo cpu{VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO};
    cpu.semaphoreCount = 1;
    cpu.pSemaphores = &semaphore;
    cpu.pValues = &target;
    vkCheck(vkWaitSemaphores(c.device, &cpu, UINT64_MAX));
    uint64_t counter{};
    vkCheck(vkGetSemaphoreCounterValue(c.device, semaphore, &counter));
    require(counter >= 2, "Timeline did not advance");
    vkCheck(vkQueueWaitIdle(c.queue));
    vkDestroySemaphore(c.device, semaphore, nullptr);
    std::cout << "Verified timeline producer 1 -> consumer 2, counter=" << counter << '\n';
}
// timestampValidBits 和 timestampPeriod 决定 tick 的解释，不把单次实验当 API 性能排名。
void timestamps(const VkContextRef& c) {
    uint32_t count{};
    vkGetPhysicalDeviceQueueFamilyProperties(c.physical, &count, nullptr);
    std::vector<VkQueueFamilyProperties> families(count);
    vkGetPhysicalDeviceQueueFamilyProperties(c.physical, &count, families.data());
    require(families[c.family].timestampValidBits != 0,
            "Selected queue does not support timestamps");
```

### 同主题跨 API 导航

| 路线 | 程序 |
| --- | --- |
| Vulkan 原生 | [vk26_timeline_counter](README.md) |
| 对照后端 | [dx26_timeline_counter](../../../../DirectX12/lessons/dx26_timeline_counter/README.md) |

### 先预测，再验证

说明 host 等 counter=2 与 GPU queue 等 counter=1 的区别。

参考推导：前者阻塞 CPU 到某进度，后者让 GPU 工作在依赖成立后执行，CPU 可以继续录制其他工作。等待对象相同不意味着阻塞同一执行主体。

## 验证与边界

运行 `--smoke` 执行有限自动检查。数值实验读取实际 GPU 结果，图形实验检查非空图像并可查看读回。交互重建与性能还需额外证据；窗口 renderer 当前保留串行等待，双槽计算实验不会被描述为完整多帧呈现引擎。
