# vk16_push_constants · 小常量计算

[54 课目录](../../COURSE.md)

## 目的与实际执行

实验用 16 字节 push constants 传 mode/count/factor/bias。pipeline layout 的 range、stageFlags 与 vkCmdPushConstants 的更新范围必须兼容。即使数据很小，也不能忽略设备 maxPushConstantsSize。

DescriptorSetLayoutBinding 连接 shader binding 与资源类型/阶段；PipelineLayout 组合 set layout 和 push range。它不是 descriptor pool，也不拥有 buffer。绑定错误不能通过只更新 buffer 内容修复。

push 常量使用字节 offset/size，并与 pipeline layout 的阶段范围相容。已有图形 Draw 为 80 字节，计算 Operation 为 16 字节；两个 layout 可以有不同约定，不应混用。

本程序有独立实验选择与验证路径，公共实现集中在 advanced_labs.cpp。设备/API 检查、数值 compute 与图形渲染采用不同证据；不是每个程序都会弹出窗口。

## 构建和运行

```powershell
cmake --preset vulkan-native
cmake --build --preset vulkan-native-debug --target vk16_push_constants --parallel 4
.\Vulkan\native\build\bin\Debug\vk16_push_constants.exe
```

## 深入学习：数据、接口与执行

- [第 16 课：Pipeline Layout、根签名与小常量](../../chapters/16_pipeline_interface/README.md)
- [第 29 课：描述符布局与资源接口声明](../../chapters/29_descriptor_layout/README.md)
- [第 32 课：Push/Root Constants 的单位与边界](../../chapters/32_per_draw_constants/README.md)

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
| Vulkan 原生 | [vk16_push_constants](README.md) |
| 对照后端 | [dx16_push_constants](../../../../DirectX12/lessons/dx16_push_constants/README.md) |

### 先预测，再验证

把 factor/bias 改为 9/17，如何证明 GPU 确实收到了新常量？

参考推导：逐项检查 result[i]=9*i+17，而不是只看 CPU 断点中的变量。结果错时核对参数顺序、大小、阶段和入口。

为一个每帧相机、每材质纹理、每对象 model 的场景设计更新频率划分。

参考推导：按帧更新 Frame，按材质选择纹理引用，按 draw 更新 model 小常量。不要每 draw 重建全部布局，也不把大型场景数据塞进 push/root constants。

加入第二个对象，仅改变它的 model，说明不需要重建哪些长期对象。

参考推导：静态几何、纹理、layout 和同一兼容 pipeline 可以复用，只在对应 draw 前更新小常量。更改不可动态状态或 shader 才可能要求另一 pipeline。

## 验证与边界

运行 `--smoke` 执行有限自动检查。数值实验读取实际 GPU 结果，图形实验检查非空图像并可查看读回。交互重建与性能还需额外证据；窗口 renderer 当前保留串行等待，双槽计算实验不会被描述为完整多帧呈现引擎。
