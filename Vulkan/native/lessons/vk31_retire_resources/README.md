# vk31_retire_resources · 完成后回收资源

[54 课目录](../../COURSE.md)

## 目的与实际执行

Buffer 销毁后才能释放它绑定的 memory；依赖 Device 的对象先于 Device 结束。command pool、descriptor pool 和 image view 也各有使用期。vkDeviceWaitIdle 可作为教学退出边界，但逐资源 fence/timeline 可以更精确地回收。

本程序有独立实验选择与验证路径，公共实现集中在 advanced_labs.cpp。设备/API 检查、数值 compute 与图形渲染采用不同证据；不是每个程序都会弹出窗口。

## 构建和运行

```powershell
cmake --preset vulkan-native
cmake --build --preset vulkan-native-debug --target vk31_retire_resources --parallel 4
.\Vulkan\native\build\bin\Debug\vk31_retire_resources.exe
```

## 深入学习：数据、接口与执行

- [第 03 课：应用结构、RAII 与 GPU 使用期](../../chapters/03_lifetime/README.md)

### 本课的真实源码切片

来自 [Vulkan/native/common/advanced_labs.cpp](../../common/advanced_labs.cpp#L166)，第 166—209 行；这是现有实现的局部摘录，末尾可能处于函数中间，不是可单独编译的完整程序。

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

### 同主题跨 API 导航

| 路线 | 程序 |
| --- | --- |
| Vulkan 原生 | [vk31_retire_resources](README.md) |
| 对照后端 | [dx31_retire_resources](../../../../DirectX12/lessons/dx31_retire_resources/README.md) |

### 先预测，再验证

将临时 upload 的回收点标到时间图上；指出“Execute/Submit 返回”之后还缺哪条证据。

参考推导：需要该资源最后一次使用所对应的 GPU 完成条件。提交返回仅说明工作已交给执行系统，不说明 copy 或 draw 已读完。

## 验证与边界

运行 `--smoke` 执行有限自动检查。数值实验读取实际 GPU 结果，图形实验检查非空图像并可查看读回。交互重建与性能还需额外证据；窗口 renderer 当前保留串行等待，双槽计算实验不会被描述为完整多帧呈现引擎。
