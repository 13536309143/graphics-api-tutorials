# 第 07 课 · 队列族、命令队列与并行边界 · Vulkan 原生

[54 课目录](../../COURSE.md) · [对照精讲](../../../../docs/compare/09_commands_frames.md) · [上一课](../06_physical_device/README.md) · [下一课](../08_logical_device/README.md)

## 本课目标与先修

学习目标：解释“队列族、命令队列与并行边界”解决的问题，找到实际对象和参数，运行对应实验并用数据/画面证明。先完成前一课，并能说明它交付的对象与完成条件；本课在该依赖上增加新契约。

## 为什么需要这一层

队列描述可提交的操作和执行通道，不直接等价于硬件独立引擎。更多队列不保证更多并行；先以单队列建立正确依赖，再判断额外队列是否值得承担同步和所有权成本。

## Vulkan 原生 的对象、参数与执行推导

queue family 具有 flags、queueCount 和 timestampValidBits。graphics/compute family 隐含支持传输，但 present support 需要对实际 surface 查询。跨 family 使用 EXCLUSIVE 资源还涉及所有权转移，不能只添加 semaphore。

## 对照 DirectX 12：相同问题的另一种分工

DIRECT、COMPUTE、COPY 队列有不同可执行操作集合。当前课程以 DIRECT 为基线，输出 queue desc；跨队列需要 Signal/Wait 建立生产消费，resource state 仍要合法。本课的计数同步不冒充独立 copy engine 优化。

## 关键参数与可检查不变量

| 检查项 | Vulkan 表达 | DX12 表达 | 不变量 / 证据 |
| --- | --- | --- | --- |
| 队列选择 | queueFamilyIndex + flags | DIRECT/COMPUTE/COPY 类型 | 命令必须被支持该操作的队列执行 |
| 命令存储归属 | pool 关联 family | allocator/list type | 不能提交不兼容命令 |
| 并行能力 | 多个 queue 不保证独立硬件 | 多个 queue 不保证独立引擎 | 实际重叠需要计时证据 |

先为每项指出源码中的创建/绑定位置，以及证明它成立的实际日志、字节或图像。数值中的设备限制以查询为准；固定实验尺寸不当作所有设备的上限。

## nvpro_core2 框架对照

ContextInitInfo 中声明队列需求，getQueueInfo 返回 queue 与 familyIndex。自定义 helper 要使用对应 family 创建 pool，而不是默认所有 GPU 都是 family 0。

这部分是原生 Vulkan 各课的框架对照，框架可运行入口仍为原有六课。如果本节需要新接入工作，会按正文说明；不把原生实验当作已完成的框架实现。

## 运行与实际检查

| 路线 | 实际实验 |
| --- | --- |
| Vulkan 原生 | [vk39_queue_properties](../../lessons/vk39_queue_properties/README.md) |
| DirectX 12 | [dx39_queue_properties](../../../../DirectX12/lessons/dx39_queue_properties/README.md) |

从合集根目录执行；普通图形程序 Esc 退出，设备/计算/诊断程序输出结果后结束。从“开始学习.cmd”进入本页时，表格可直接点击运行。

```powershell
cmake --preset vulkan-native
cmake --build --preset vulkan-native-debug --target vk39_queue_properties --parallel 4
.\Vulkan\native\build\bin\Debug\vk39_queue_properties.exe
ctest --preset vulkan-native-debug -R "^vk39_queue_properties$" --output-on-failure
```

检查三个层次：创建/绑定/提交的接口约束；数据读回或目标画面；资源使用结束后的回收。诊断实验用合法信息或查询证明渠道可用，不故意保留 API 错误。图形 smoke 的像素检查与数值实验的逐项检查不同，具体实现见源码。

## 关键实现：从当前源码定位

来自 [Vulkan/native/common/advanced_labs.cpp](../../common/advanced_labs.cpp#L284)，第 284—327 行。下列是当前仓库的局部切片，可能在函数中间结束；完整上下文请打开文件，切片不作为单独可编译程序。

```cpp
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
```

阅读时按“前置对象 → 参数/布局 → 录制/调用 → 完成/可见性 → 释放”五步标注。共享 helper 减少重复代码，但不会取消每课的 API 契约。

## 受控修改实验

说明为什么同一 queue 上顺序提交，仍可能需要资源内存屏障。

先记录预测，再只改一个因素；保留基线与修改后的输出。不要同时改变 shader、资源范围和同步，这样即使结果变好也无法归因。

### 参考推导与通过标准

提交顺序不自动提供每一种访问之间所需的可见性或布局/状态变化。要从生产者与消费者推导依赖，不能用“排在后面”替代规则。

## 常见失败与排查顺序

把 queue index 与 queue family index 混淆，会使 command pool 与提交队列不匹配。

先看第一条错误和当前执行路径，再看实际数据/绑定，最后看 shader 与画面。需要 GPU 证据时抓取正常交互帧；仅四帧的 smoke 适合自动检查，不适合手动抓帧。

## 自测与本课交付

能否不看源码回答：说明为什么同一 queue 上顺序提交，仍可能需要资源内存屏障。

提交一份简短记录：输入与假设、唯一修改、预期、实际证据、原因、是否恢复基线。如果只能复述函数名而说不清何时允许访问/释放，先回到本课的数据流。

## 官方阅读位置与当前工程差异

[指定 Vulkan 教程对应位置](https://docs.vulkan.net.cn/tutorial/latest/03_Drawing_a_triangle/00_Setup/03_Physical_devices_and_queue_families.html)。DX12 路线按同一概念顺序建立映射，其对象与约束以 Microsoft 文档和本后端代码为准。49—54 课包含在基础/compute 之后加入的工程扩展。

[版本与资料说明](../../../../docs/REFERENCES.md) · [调试](../../../../docs/DEBUGGING.md) · [功能完成度](../../../../docs/FEATURE_MATRIX.md)
