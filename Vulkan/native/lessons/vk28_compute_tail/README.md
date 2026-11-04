# vk28_compute_tail · 非整组尾部计算

[54 课目录](../../COURSE.md)

## 目的与实际执行

storage buffer range 覆盖足够字节，shader 以 i<count 防止尾部写越界。host 用向上取整组数并逐项核对 affine 公式。buffer 的 usage、descriptor 类型与 shader 声明必须相容。

本程序有独立实验选择与验证路径，公共实现集中在 advanced_labs.cpp。设备/API 检查、数值 compute 与图形渲染采用不同证据；不是每个程序都会弹出窗口。

## 构建和运行

```powershell
cmake --preset vulkan-native
cmake --build --preset vulkan-native-debug --target vk28_compute_tail --parallel 4
.\Vulkan\native\build\bin\Debug\vk28_compute_tail.exe
```

## 深入学习：数据、接口与执行

- [第 47 课：Storage/UAV、边界与 257 项实验](../../chapters/47_storage_tail/README.md)

### 本课的真实源码切片

来自 [Vulkan/native/common/advanced_labs.cpp](../../common/advanced_labs.cpp#L114)，第 114—157 行；这是现有实现的局部摘录，末尾可能处于函数中间，不是可单独编译的完整程序。

```cpp
    void record(VkCommandBuffer cmd,
                uint32_t mode,
                uint32_t count,
                uint32_t factor,
                uint32_t bias,
                uint32_t slot = 0) {
        uint32_t constants[4] = {mode, count, factor, bias};
        uint32_t offset = slot * stride;
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
        vkCmdBindDescriptorSets(
            cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout, 0, 1, &set, 1, &offset);
        vkCmdPushConstants(cmd, pipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, 16, constants);
        vkCmdDispatch(cmd, (count + 63) / 64, 1, 1);
        bufferDependency(cmd,
                         output.buffer,
                         VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                         VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
                         VK_PIPELINE_STAGE_2_HOST_BIT,
                         VK_ACCESS_2_HOST_READ_BIT);
    }
    void execute(uint32_t mode, uint32_t count, uint32_t factor, uint32_t bias, uint32_t slot = 0) {
        immediate(c, [&](VkCommandBuffer cmd) {
            record(cmd, mode, count, factor, bias, slot);
        });
    }
    ~ComputeKernel() {
        vkDestroyPipeline(c.device, pipeline, nullptr);
        vkDestroyPipelineLayout(c.device, pipelineLayout, nullptr);
        vkDestroyDescriptorPool(c.device, pool, nullptr);
        vkDestroyDescriptorSetLayout(c.device, layout, nullptr);
    }
};
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
```

### 同主题跨 API 导航

| 路线 | 程序 |
| --- | --- |
| Vulkan 原生 | [vk28_compute_tail](README.md) |
| 对照后端 | [dx28_compute_tail](../../../../DirectX12/lessons/dx28_compute_tail/README.md) |

### 先预测，再验证

只把 CPU 验证改成 257 而仍分配 256 项，会产生什么问题？

参考推导：可能读取越界；只扩容量却不扩组数则尾项未写。正确验收不能用忽略最后一项来让测试通过。

## 验证与边界

运行 `--smoke` 执行有限自动检查。数值实验读取实际 GPU 结果，图形实验检查非空图像并可查看读回。交互重建与性能还需额外证据；窗口 renderer 当前保留串行等待，双槽计算实验不会被描述为完整多帧呈现引擎。
