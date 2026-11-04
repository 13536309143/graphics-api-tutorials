# vk13_staging_copy · 设备内存上传与读回

[54 课目录](../../COURSE.md)

## 目的与实际执行

源/目标分别声明 TRANSFER_SRC/DST，设备缓冲选择 device-local。第一次 copy 的写入通过 buffer barrier 对第二次 transfer read 可见，最后 copy 写入经 host-read 依赖供 CPU 检查。

Image 创建声明 usage/format/extent/mip，MemoryRequirements 决定绑定存储。BufferImageCopy 描述缓冲到子资源的映射；先处于 TRANSFER_DST，再允许采样。创建 view 或更新 descriptor 不会替代 copy。

copy-write 到 fragment-sampled-read 需要对应 stage/access 和布局转换。mip 生成逐级把前一级从 transfer-dst 切成 transfer-src，最终各级转 shader-read；不能仅在整个流程结束后随意改一个枚举。

本程序有独立实验选择与验证路径，公共实现集中在 advanced_labs.cpp。设备/API 检查、数值 compute 与图形渲染采用不同证据；不是每个程序都会弹出窗口。

## 构建和运行

```powershell
cmake --preset vulkan-native
cmake --build --preset vulkan-native-debug --target vk13_staging_copy --parallel 4
.\Vulkan\native\build\bin\Debug\vk13_staging_copy.exe
```

## 深入学习：数据、接口与执行

- [第 26 课：暂存上传、设备资源与读回闭环](../../chapters/26_staging/README.md)
- [第 34 课：图像存储、Footprint 与拷贝](../../chapters/34_image_upload/README.md)
- [第 35 课：图像布局、资源状态与读写依赖](../../chapters/35_image_dependencies/README.md)

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
| Vulkan 原生 | [vk13_staging_copy](README.md) |
| 对照后端 | [dx13_staging_copy](../../../../DirectX12/lessons/dx13_staging_copy/README.md) |

### 先预测，再验证

若最终只检查一个元素，哪些错误可能漏掉？

参考推导：尾部未拷贝、偏移偏一、局部范围错误仍可能让首元素正确。闭环应核对全部元素和容量，再把路径用于顶点或纹理。

解释一次 memcpy(width*height*4) 为什么不总能正确填 texture upload。

参考推导：API 可能要求更大的对齐行距或子资源起点。正确内容应逐行写到 footprint 的 Offset+y*RowPitch，再按有效宽度复制。

把纹理消费者从 pixel/fragment 改为 compute，哪些同步描述也要一起改变？

参考推导：消费者阶段/访问或 state 应反映 compute 读取。直接复用只针对片元的依赖不一定覆盖新的使用者。

## 验证与边界

运行 `--smoke` 执行有限自动检查。数值实验读取实际 GPU 结果，图形实验检查非空图像并可查看读回。交互重建与性能还需额外证据；窗口 renderer 当前保留串行等待，双槽计算实验不会被描述为完整多帧呈现引擎。
