# 第 53 课 · GPU 时间戳、管线缓存与测量边界 · Vulkan 原生

[54 课目录](../../COURSE.md) · [对照精讲](../../../../docs/compare/14_debug_performance.md) · [上一课](../52_camera_material/README.md) · [下一课](../54_capstone/README.md)

## 本课目标与先修

学习目标：解释“GPU 时间戳、管线缓存与测量边界”解决的问题，找到实际对象和参数，运行对应实验并用数据/画面证明。先完成前一课，并能说明它交付的对象与完成条件；本课在该依赖上增加新契约。

## 为什么需要这一层

这是工程扩展。时间戳观察 GPU 某段工作，CPU 调用耗时观察录制/提交；二者不能互相替代。缓存实验验证序列化和重建后结果一致，不以一次运行就宣称某 API 更快。

## Vulkan 原生 的对象、参数与执行推导

timestampValidBits 与 timestampPeriod 决定计时解释；本例记录填充操作前后时间并处理有效位。pipeline cache 在同一设备/驱动条件下保存并重建 compute pipeline，实际执行后再核对数值。

## 对照 DirectX 12：相同问题的另一种分工

queue timestamp frequency 将 query ticks 转成时间，结果经 ResolveQueryData/readback 等待后读取。缓存 PSO 重建复用完全相同的编译字节码；Debug 重编译可能引入变化，不能仅凭源码相同断言 cache 一定兼容。

## 关键参数与可检查不变量

| 检查项 | Vulkan 表达 | DX12 表达 | 不变量 / 证据 |
| --- | --- | --- | --- |
| 计时单位 | 有效 bits + period | ticks / queue frequency | 分别解释 GPU 与 CPU 耗时 |
| 缓存输入 | 相容 pipeline 与 cache bytes | 相同 DXBC 与 PSO 描述 | 不是所有版本/驱动可通用数据 |
| 结论范围 | 单操作/重建验证 | 单操作/重建验证 | 不是跨 API benchmark |

先为每项指出源码中的创建/绑定位置，以及证明它成立的实际日志、字节或图像。数值中的设备限制以查询为准；固定实验尺寸不当作所有设备的上限。

## nvpro_core2 框架对照

UI、离屏与框架调度应从场景 pass 测量中区分。工具抓帧的额外开销也不等于发布版正常帧成本。

这部分是原生 Vulkan 各课的框架对照，框架可运行入口仍为原有六课。如果本节需要新接入工作，会按正文说明；不把原生实验当作已完成的框架实现。

## 运行与实际检查

| 路线 | 实际实验 |
| --- | --- |
| Vulkan 原生 | [vk25_timestamps](../../lessons/vk25_timestamps/README.md) |
| Vulkan 原生 | [vk32_pipeline_cache](../../lessons/vk32_pipeline_cache/README.md) |
| DirectX 12 | [dx25_timestamps](../../../../DirectX12/lessons/dx25_timestamps/README.md) |
| DirectX 12 | [dx32_pipeline_cache](../../../../DirectX12/lessons/dx32_pipeline_cache/README.md) |

从合集根目录执行；普通图形程序 Esc 退出，设备/计算/诊断程序输出结果后结束。从“开始学习.cmd”进入本页时，表格可直接点击运行。

```powershell
cmake --preset vulkan-native
cmake --build --preset vulkan-native-debug --target vk25_timestamps --parallel 4
.\Vulkan\native\build\bin\Debug\vk25_timestamps.exe
ctest --preset vulkan-native-debug -R "^vk25_timestamps$" --output-on-failure
```

检查三个层次：创建/绑定/提交的接口约束；数据读回或目标画面；资源使用结束后的回收。诊断实验用合法信息或查询证明渠道可用，不故意保留 API 错误。图形 smoke 的像素检查与数值实验的逐项检查不同，具体实现见源码。

## 关键实现：从当前源码定位

来自 [Vulkan/native/common/advanced_labs.cpp](../../common/advanced_labs.cpp#L290)，第 290—333 行。下列是当前仓库的局部切片，可能在函数中间结束；完整上下文请打开文件，切片不作为单独可编译程序。

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

阅读时按“前置对象 → 参数/布局 → 录制/调用 → 完成/可见性 → 释放”五步标注。共享 helper 减少重复代码，但不会取消每课的 API 契约。

## 受控修改实验

列出一个公平跨 API 性能实验至少需要固定的六项条件。

先记录预测，再只改一个因素；保留基线与修改后的输出。不要同时改变 shader、资源范围和同步，这样即使结果变好也无法归因。

### 参考推导与通过标准

工作量、分辨率、shader/材质、场景/相机、采样/呈现策略、构建配置；还应预热、重复采样并报告 CPU/GPU/整帧时间及波动。

## 常见失败与排查顺序

把 Debug smoke 的总秒数、垂直同步限制的 FPS 或一次最小值当成 API 性能排名，均缺少可比前提。

先看第一条错误和当前执行路径，再看实际数据/绑定，最后看 shader 与画面。需要 GPU 证据时抓取正常交互帧；仅四帧的 smoke 适合自动检查，不适合手动抓帧。

## 自测与本课交付

能否不看源码回答：列出一个公平跨 API 性能实验至少需要固定的六项条件。

提交一份简短记录：输入与假设、唯一修改、预期、实际证据、原因、是否恢复基线。如果只能复述函数名而说不清何时允许访问/释放，先回到本课的数据流。

## 官方阅读位置与当前工程差异

[指定 Vulkan 教程对应位置](https://docs.vulkan.net.cn/tutorial/latest/11_Compute_Shader.html)。DX12 路线按同一概念顺序建立映射，其对象与约束以 Microsoft 文档和本后端代码为准。49—54 课包含在基础/compute 之后加入的工程扩展。

[版本与资料说明](../../../../docs/REFERENCES.md) · [调试](../../../../docs/DEBUGGING.md) · [功能完成度](../../../../docs/FEATURE_MATRIX.md)
