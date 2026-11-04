# 第 03 课 · 应用结构、RAII 与 GPU 使用期 · Vulkan 原生

[54 课目录](../../COURSE.md) · [对照精讲](../../../../docs/compare/09_commands_frames.md) · [上一课](../02_environment/README.md) · [下一课](../04_instance_factory/README.md)

## 本课目标与先修

学习目标：解释“应用结构、RAII 与 GPU 使用期”解决的问题，找到实际对象和参数，运行对应实验并用数据/画面证明。先完成前一课，并能说明它交付的对象与完成条件；本课在该依赖上增加新契约。

## 为什么需要这一层

CPU 对象的作用域与 GPU 工作的使用期是两条时间线。析构能够释放 C++ 拥有的资源，却不能自动证明最后一次提交已完成。先定义拥有关系，再把 GPU 完成标记作为回收前置条件。

本实验重复创建上传、设备和读回资源，每代提交完成并核对结果后再退出作用域。这是容易验证的安全基线，不是高吞吐延迟回收器。工程化时应把“本代资源 + 最后使用完成值”放入回收队列，等待完成值越过后回收。

## Vulkan 原生 的对象、参数与执行推导

Buffer 销毁后才能释放它绑定的 memory；依赖 Device 的对象先于 Device 结束。command pool、descriptor pool 和 image view 也各有使用期。vkDeviceWaitIdle 可作为教学退出边界，但逐资源 fence/timeline 可以更精确地回收。

## 对照 DirectX 12：相同问题的另一种分工

ComPtr 只控制 COM 引用计数；最后一个引用释放时若 GPU 还在用资源，仍然错误。UPLOAD 的 CPU 字节也不能在 copy 未完成时覆盖。fence 完成值同时保护 resource、descriptor 范围和 command allocator 的回收。

## 关键参数与可检查不变量

| 检查项 | Vulkan 表达 | DX12 表达 | 不变量 / 证据 |
| --- | --- | --- | --- |
| CPU 所有权 | RAII 包装句柄 | ComPtr 引用计数 | CPU 析构不等于 GPU 完成 |
| 回收条件 | 最后一次提交的 fence/timeline | 最后使用的 fence value | 完成后才回收 upload、descriptor、命令存储 |
| 本实验范围 | 四代串行闭环 | 四代串行闭环 | 不是已实现异步垃圾回收器 |

先为每项指出源码中的创建/绑定位置，以及证明它成立的实际日志、字节或图像。数值中的设备限制以查询为准；固定实验尺寸不当作所有设备的上限。

## nvpro_core2 框架对照

ResourceAllocator 的 destroyBuffer 仍要求调用方确保无在途引用。onDetach 应先等待 GPU，再释放依赖 allocator 的对象，最后 deinit allocator。

这部分是原生 Vulkan 各课的框架对照，框架可运行入口仍为原有六课。如果本节需要新接入工作，会按正文说明；不把原生实验当作已完成的框架实现。

## 运行与实际检查

| 路线 | 实际实验 |
| --- | --- |
| Vulkan 原生 | [vk31_retire_resources](../../lessons/vk31_retire_resources/README.md) |
| DirectX 12 | [dx31_retire_resources](../../../../DirectX12/lessons/dx31_retire_resources/README.md) |

从合集根目录执行；普通图形程序 Esc 退出，设备/计算/诊断程序输出结果后结束。从“开始学习.cmd”进入本页时，表格可直接点击运行。

```powershell
cmake --preset vulkan-native
cmake --build --preset vulkan-native-debug --target vk31_retire_resources --parallel 4
.\Vulkan\native\build\bin\Debug\vk31_retire_resources.exe
ctest --preset vulkan-native-debug -R "^vk31_retire_resources$" --output-on-failure
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

将临时 upload 的回收点标到时间图上；指出“Execute/Submit 返回”之后还缺哪条证据。

先记录预测，再只改一个因素；保留基线与修改后的输出。不要同时改变 shader、资源范围和同步，这样即使结果变好也无法归因。

### 参考推导与通过标准

需要该资源最后一次使用所对应的 GPU 完成条件。提交返回仅说明工作已交给执行系统，不说明 copy 或 draw 已读完。

## 常见失败与排查顺序

调试器里 C++ 指针不为空不能证明 GPU 句柄仍有效；某个驱动偶尔未崩溃也不能证明回收顺序合法。

先看第一条错误和当前执行路径，再看实际数据/绑定，最后看 shader 与画面。需要 GPU 证据时抓取正常交互帧；仅四帧的 smoke 适合自动检查，不适合手动抓帧。

## 自测与本课交付

能否不看源码回答：将临时 upload 的回收点标到时间图上；指出“Execute/Submit 返回”之后还缺哪条证据。

提交一份简短记录：输入与假设、唯一修改、预期、实际证据、原因、是否恢复基线。如果只能复述函数名而说不清何时允许访问/释放，先回到本课的数据流。

## 官方阅读位置与当前工程差异

[指定 Vulkan 教程对应位置](https://docs.vulkan.net.cn/tutorial/latest/03_Drawing_a_triangle/00_Setup/00_Base_code.html)。DX12 路线按同一概念顺序建立映射，其对象与约束以 Microsoft 文档和本后端代码为准。49—54 课包含在基础/compute 之后加入的工程扩展。

[版本与资料说明](../../../../docs/REFERENCES.md) · [调试](../../../../docs/DEBUGGING.md) · [功能完成度](../../../../docs/FEATURE_MATRIX.md)
