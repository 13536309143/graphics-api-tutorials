# 第 35 课 · 图像布局、资源状态与读写依赖 · Vulkan 原生

[54 课目录](../../COURSE.md) · [对照精讲](../../../../docs/compare/08_synchronization.md) · [上一课](../34_image_upload/README.md) · [下一课](../36_samplers/README.md)

## 本课目标与先修

学习目标：解释“图像布局、资源状态与读写依赖”解决的问题，找到实际对象和参数，运行对应实验并用数据/画面证明。先完成前一课，并能说明它交付的对象与完成条件；本课在该依赖上增加新契约。

## 为什么需要这一层

为每次访问写下生产者与消费者：谁写、谁读、在哪个队列、读写范围是什么。布局/状态表示合法用途的一部分，执行顺序与可见性还要通过同步规则建立。

## Vulkan 原生 的对象、参数与执行推导

copy-write 到 fragment-sampled-read 需要对应 stage/access 和布局转换。mip 生成逐级把前一级从 transfer-dst 切成 transfer-src，最终各级转 shader-read；不能仅在整个流程结束后随意改一个枚举。

## 对照 DirectX 12：相同问题的另一种分工

legacy transition barrier 表达 COPY_DEST、UAV、shader resource 等用途转换。compute mip 每级有自己的 subresource state；UAV 写后下级采样的依赖由对应转换建立。跨队列还需 fence Signal/Wait。

## 关键参数与可检查不变量

| 检查项 | Vulkan 表达 | DX12 表达 | 不变量 / 证据 |
| --- | --- | --- | --- |
| 生产者 | COPY/BLIT write | COPY_DEST 或 UAV write | 写入来源决定依赖 |
| 消费者 | fragment/compute sampled read | pixel/non-pixel shader read | 按真实使用者选择范围 |
| 每级状态 | 逐 mip layout/access | 逐 subresource state | 不假定全纹理始终同一状态 |

先为每项指出源码中的创建/绑定位置，以及证明它成立的实际日志、字节或图像。数值中的设备限制以查询为准；固定实验尺寸不当作所有设备的上限。

## nvpro_core2 框架对照

GENERAL→GENERAL 也可能需要依赖。RenderTarget 与 ImGui 的颜色写后采样正是布局不变而访问变化的例子。

这部分是原生 Vulkan 各课的框架对照，框架可运行入口仍为原有六课。如果本节需要新接入工作，会按正文说明；不把原生实验当作已完成的框架实现。

## 运行与实际检查

| 路线 | 实际实验 |
| --- | --- |
| Vulkan 原生 | [vk13_staging_copy](../../lessons/vk13_staging_copy/README.md) |
| Vulkan 原生 | [vk18_mip_chain](../../lessons/vk18_mip_chain/README.md) |
| DirectX 12 | [dx13_staging_copy](../../../../DirectX12/lessons/dx13_staging_copy/README.md) |
| DirectX 12 | [dx18_mip_chain](../../../../DirectX12/lessons/dx18_mip_chain/README.md) |

从合集根目录执行；普通图形程序 Esc 退出，设备/计算/诊断程序输出结果后结束。从“开始学习.cmd”进入本页时，表格可直接点击运行。

```powershell
cmake --preset vulkan-native
cmake --build --preset vulkan-native-debug --target vk13_staging_copy --parallel 4
.\Vulkan\native\build\bin\Debug\vk13_staging_copy.exe
ctest --preset vulkan-native-debug -R "^vk13_staging_copy$" --output-on-failure
```

检查三个层次：创建/绑定/提交的接口约束；数据读回或目标画面；资源使用结束后的回收。诊断实验用合法信息或查询证明渠道可用，不故意保留 API 错误。图形 smoke 的像素检查与数值实验的逐项检查不同，具体实现见源码。

## 关键实现：从当前源码定位

来自 [Vulkan/native/common/advanced_labs.cpp](../../common/advanced_labs.cpp#L166)，第 166—209 行。下列是当前仓库的局部切片，可能在函数中间结束；完整上下文请打开文件，切片不作为单独可编译程序。

```cpp
void stagingCopy(const VkContextRef& c) {
    constexpr size_t count = 257;
    std::vector<uint32_t> original(count);
    for (uint32_t i = 0; i < count; ++i)
        original[i] = i * 17 + 9;
    VkBufferResource upload, gpu, readback;
    upload.init(c, count * 4, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
    upload.upload(original.data(), count * 4);
    gpu.init(c,
             count * 4,
             VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
             VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    readback.init(c, count * 4, VK_BUFFER_USAGE_TRANSFER_DST_BIT);
    immediate(c, [&](VkCommandBuffer cmd) {
        VkBufferCopy copy{0, 0, count * 4};
        vkCmdCopyBuffer(cmd, upload.buffer, gpu.buffer, 1, &copy);
        bufferDependency(cmd,
                         gpu.buffer,
                         VK_PIPELINE_STAGE_2_COPY_BIT,
                         VK_ACCESS_2_TRANSFER_WRITE_BIT,
                         VK_PIPELINE_STAGE_2_COPY_BIT,
                         VK_ACCESS_2_TRANSFER_READ_BIT);
        vkCmdCopyBuffer(cmd, gpu.buffer, readback.buffer, 1, &copy);
        bufferDependency(cmd,
                         readback.buffer,
                         VK_PIPELINE_STAGE_2_COPY_BIT,
                         VK_ACCESS_2_TRANSFER_WRITE_BIT,
                         VK_PIPELINE_STAGE_2_HOST_BIT,
                         VK_ACCESS_2_HOST_READ_BIT);
    });
    require(readWords(readback, count) == original, "Device-local staging copy mismatch");
    std::cout << "Verified 257 device-local round-trip words\n";
}
// 两份独立资源交替在途，重用前等待对应 fence；这不是窗口交换链的完整多帧实现。
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
```

阅读时按“前置对象 → 参数/布局 → 录制/调用 → 完成/可见性 → 释放”五步标注。共享 helper 减少重复代码，但不会取消每课的 API 契约。

## 受控修改实验

把纹理消费者从 pixel/fragment 改为 compute，哪些同步描述也要一起改变？

先记录预测，再只改一个因素；保留基线与修改后的输出。不要同时改变 shader、资源范围和同步，这样即使结果变好也无法归因。

### 参考推导与通过标准

消费者阶段/访问或 state 应反映 compute 读取。直接复用只针对片元的依赖不一定覆盖新的使用者。

## 常见失败与排查顺序

coherent、Map 成功、descriptor 中的 layout 或 CPU 提交顺序，任何一个都不能独自证明完整 GPU 写后读关系。

先看第一条错误和当前执行路径，再看实际数据/绑定，最后看 shader 与画面。需要 GPU 证据时抓取正常交互帧；仅四帧的 smoke 适合自动检查，不适合手动抓帧。

## 自测与本课交付

能否不看源码回答：把纹理消费者从 pixel/fragment 改为 compute，哪些同步描述也要一起改变？

提交一份简短记录：输入与假设、唯一修改、预期、实际证据、原因、是否恢复基线。如果只能复述函数名而说不清何时允许访问/释放，先回到本课的数据流。

## 官方阅读位置与当前工程差异

[指定 Vulkan 教程对应位置](https://docs.vulkan.net.cn/tutorial/latest/06_Texture_mapping/00_Images.html)。DX12 路线按同一概念顺序建立映射，其对象与约束以 Microsoft 文档和本后端代码为准。49—54 课包含在基础/compute 之后加入的工程扩展。

[版本与资料说明](../../../../docs/REFERENCES.md) · [调试](../../../../docs/DEBUGGING.md) · [功能完成度](../../../../docs/FEATURE_MATRIX.md)
