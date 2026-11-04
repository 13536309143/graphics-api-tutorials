# vk11_capabilities · 能力与格式查询

[54 课目录](../../COURSE.md)

## 目的与实际执行

Instance 管理应用与实现的连接；PhysicalDevice 描述能力；Device 启用选择的能力；Queue 接收提交。本工程用 Vulkan 1.3 C API、dynamic rendering 和 sync2。参考教程中传统 RenderPass/Framebuffer 会对照解释，本工程不因此伪称使用了那套对象。

PhysicalDeviceProperties 提供 limits，MemoryProperties 描述 heap/type。格式还要单独查 optimal tiling features。实验检查 80 字节 Draw 的 push constant 容量及 D32 深度用途，同时输出 UBO 对齐、工作组限制和内存类型。

本程序有独立实验选择与验证路径，公共实现集中在 advanced_labs.cpp。设备/API 检查、数值 compute 与图形渲染采用不同证据；不是每个程序都会弹出窗口。

## 构建和运行

```powershell
cmake --preset vulkan-native
cmake --build --preset vulkan-native-debug --target vk11_capabilities --parallel 4
.\Vulkan\native\build\bin\Debug\vk11_capabilities.exe
```

## 深入学习：数据、接口与执行

- [第 01 课：图形 API 的职责与课程地图](../../chapters/01_architecture/README.md)
- [第 06 课：物理设备、Adapter 与格式能力](../../chapters/06_physical_device/README.md)

### 本课的真实源码切片

来自 [Vulkan/native/common/advanced_labs.cpp](../../common/advanced_labs.cpp#L146)，第 146—189 行；这是现有实现的局部摘录，末尾可能处于函数中间，不是可单独编译的完整程序。

```cpp
void capabilities(const VkContextRef& c) {
    VkPhysicalDeviceProperties p{};
    vkGetPhysicalDeviceProperties(c.physical, &p);
    VkPhysicalDeviceMemoryProperties m{};
    vkGetPhysicalDeviceMemoryProperties(c.physical, &m);
    std::cout << "maxPushConstantsSize=" << p.limits.maxPushConstantsSize
              << ", minUniformBufferOffsetAlignment=" << p.limits.minUniformBufferOffsetAlignment
              << ", maxComputeWorkGroupInvocations=" << p.limits.maxComputeWorkGroupInvocations
              << '\n';
    for (uint32_t i = 0; i < m.memoryTypeCount; ++i)
        std::cout << "memoryType[" << i << "] heap=" << m.memoryTypes[i].heapIndex
                  << " properties=" << m.memoryTypes[i].propertyFlags << '\n';
    require(p.limits.maxPushConstantsSize >= 80, "80-byte Draw exceeds device limit");
    VkFormatProperties depth{};
    vkGetPhysicalDeviceFormatProperties(c.physical, VK_FORMAT_D32_SFLOAT, &depth);
    require((depth.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) != 0,
            "D32 unsupported");
    std::cout << "Verified required limits and depth format\n";
}
// upload → device-local → readback 完整闭环；每次拷贝后的消费者访问不同。
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
```

### 同主题跨 API 导航

| 路线 | 程序 |
| --- | --- |
| Vulkan 原生 | [vk11_capabilities](README.md) |
| 对照后端 | [dx11_capabilities](../../../../DirectX12/lessons/dx11_capabilities/README.md) |

### 先预测，再验证

画出一个三角形所需的五类对象，并说明哪些对象保存字节、哪些只保存解释或引用。

参考推导：Buffer/Image 保存内容，ImageView/descriptor/布局解释或引用内容，pipeline 约定执行方式；Queue 执行已录制工作。引用不会复制资源，也不会延长 GPU 资源的合法使用期。

写出本项目基础与 MSAA 实验各自的能力清单，并标明查询位置。

参考推导：基础至少需要图形/计算、RGBA8、深度、常量范围；MSAA 还要颜色和深度共同支持同一样本数。单独颜色支持 4x 不足以建立匹配附件。

## 验证与边界

运行 `--smoke` 执行有限自动检查。数值实验读取实际 GPU 结果，图形实验检查非空图像并可查看读回。交互重建与性能还需额外证据；窗口 renderer 当前保留串行等待，双槽计算实验不会被描述为完整多帧呈现引擎。
