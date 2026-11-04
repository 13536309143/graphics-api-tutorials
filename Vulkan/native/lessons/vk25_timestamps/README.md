# vk25_timestamps · GPU 时间戳

[54 课目录](../../COURSE.md)

## 目的与实际执行

timestampValidBits 与 timestampPeriod 决定计时解释；本例记录填充操作前后时间并处理有效位。pipeline cache 在同一设备/驱动条件下保存并重建 compute pipeline，实际执行后再核对数值。

本程序有独立实验选择与验证路径，公共实现集中在 advanced_labs.cpp。设备/API 检查、数值 compute 与图形渲染采用不同证据；不是每个程序都会弹出窗口。

## 构建和运行

```powershell
cmake --preset vulkan-native
cmake --build --preset vulkan-native-debug --target vk25_timestamps --parallel 4
.\Vulkan\native\build\bin\Debug\vk25_timestamps.exe
```

## 深入学习：数据、接口与执行

- [第 53 课：GPU 时间戳、管线缓存与测量边界](../../chapters/53_measure_cache/README.md)

### 本课的真实源码切片

来自 [Vulkan/native/common/advanced_labs.cpp](../../common/advanced_labs.cpp#L290)，第 290—333 行；这是现有实现的局部摘录，末尾可能处于函数中间，不是可单独编译的完整程序。

```cpp
void timestamps(const VkContextRef& c) {
    uint32_t count{};
    vkGetPhysicalDeviceQueueFamilyProperties(c.physical, &count, nullptr);
    std::vector<VkQueueFamilyProperties> families(count);
    vkGetPhysicalDeviceQueueFamilyProperties(c.physical, &count, families.data());
    require(families[c.family].timestampValidBits != 0,
            "Selected queue does not support timestamps");
    VkQueryPoolCreateInfo info{VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO};
    info.queryType = VK_QUERY_TYPE_TIMESTAMP;
    info.queryCount = 2;
    VkQueryPool pool{};
    vkCheck(vkCreateQueryPool(c.device, &info, nullptr, &pool));
    VkBufferResource buffer;
    buffer.init(
        c, 1024 * 1024, VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    immediate(c, [&](VkCommandBuffer cmd) {
        vkCmdResetQueryPool(cmd, pool, 0, 2);
        vkCmdWriteTimestamp2(cmd, VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, pool, 0);
        vkCmdFillBuffer(cmd, buffer.buffer, 0, VK_WHOLE_SIZE, 0x12345678);
        vkCmdWriteTimestamp2(cmd, VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, pool, 1);
    });
    uint64_t ticks[2]{};
    vkCheck(vkGetQueryPoolResults(c.device,
                                  pool,
                                  0,
                                  2,
                                  sizeof(ticks),
                                  ticks,
                                  8,
                                  VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT));
    VkPhysicalDeviceProperties p{};
    vkGetPhysicalDeviceProperties(c.physical, &p);
    auto bits = families[c.family].timestampValidBits;
    uint64_t mask = bits == 64 ? UINT64_MAX : ((uint64_t(1) << bits) - 1);
    std::cout << "GPU fill measurement="
              << ((ticks[1] - ticks[0]) & mask) * p.limits.timestampPeriod / 1e6
              << " ms (single operation, not an API benchmark)\n";
    vkDestroyQueryPool(c.device, pool, nullptr);
}
void imageDependency(VkCommandBuffer cmd,
                     VkImage image,
                     uint32_t level,
                     VkImageLayout before,
                     VkImageLayout after,
```

### 同主题跨 API 导航

| 路线 | 程序 |
| --- | --- |
| Vulkan 原生 | [vk25_timestamps](README.md) |
| 对照后端 | [dx25_timestamps](../../../../DirectX12/lessons/dx25_timestamps/README.md) |

### 先预测，再验证

列出一个公平跨 API 性能实验至少需要固定的六项条件。

参考推导：工作量、分辨率、shader/材质、场景/相机、采样/呈现策略、构建配置；还应预热、重复采样并报告 CPU/GPU/整帧时间及波动。

## 验证与边界

运行 `--smoke` 执行有限自动检查。数值实验读取实际 GPU 结果，图形实验检查非空图像并可查看读回。交互重建与性能还需额外证据；窗口 renderer 当前保留串行等待，双槽计算实验不会被描述为完整多帧呈现引擎。
