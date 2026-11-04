# vk14_constant_alignment · 常量缓冲对齐

[54 课目录](../../COURSE.md)

## 目的与实际执行

memoryTypeBits 是资源允许的类型集合，propertyFlags 是内存属性。实验为 257 字节缓冲查询实际 size/alignment/type bits，再在允许集合中选择所需属性。non-coherent 的 flush/invalidate 还需符合原子尺寸对齐。

GLSL mat4 与 GLM 列主序配合，UBO 的布局必须与 C++ 实际偏移一致。动态偏移按 minUniformBufferOffsetAlignment 对齐，range 仍应覆盖具体内容且不越界。实验用 uvec4 参数让 GPU 数值验证解码。

本程序有独立实验选择与验证路径，公共实现集中在 advanced_labs.cpp。设备/API 检查、数值 compute 与图形渲染采用不同证据；不是每个程序都会弹出窗口。

## 构建和运行

```powershell
cmake --preset vulkan-native
cmake --build --preset vulkan-native-debug --target vk14_constant_alignment --parallel 4
.\Vulkan\native\build\bin\Debug\vk14_constant_alignment.exe
```

## 深入学习：数据、接口与执行

- [第 25 课：内存类型、Heap 与分配要求](../../chapters/25_memory/README.md)
- [第 28 课：常量结构、矩阵存储与对齐](../../chapters/28_uniform_layout/README.md)

### 本课的真实源码切片

来自 [Vulkan/native/common/advanced_labs.cpp](../../common/advanced_labs.cpp#L38)，第 38—81 行；这是现有实现的局部摘录，末尾可能处于函数中间，不是可单独编译的完整程序。

```cpp
struct ComputeKernel {
    VkContextRef c;
    VkBufferResource output, parameters;
    VkDescriptorSetLayout layout{};
    VkDescriptorPool pool{};
    VkDescriptorSet set{};
    VkPipelineLayout pipelineLayout{};
    VkPipeline pipeline{};
    uint32_t stride{};
    explicit ComputeKernel(VkContextRef context, VkPipelineCache cache = {}) : c(context) {
        VkPhysicalDeviceProperties props{};
        vkGetPhysicalDeviceProperties(c.physical, &props);
        stride = uint32_t(alignUp(16, props.limits.minUniformBufferOffsetAlignment));
        output.init(c, 1024 * 4, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
        parameters.init(c, stride * 3, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
        std::vector<uint8_t> bytes(stride * 3);
        const uint32_t pairs[3][4] = {{2, 3, 0, 0}, {5, 11, 0, 0}, {7, 13, 0, 0}};
        for (uint32_t i = 0; i < 3; ++i)
            std::memcpy(bytes.data() + stride * i, pairs[i], 16);
        parameters.upload(bytes.data(), bytes.size());
        VkDescriptorSetLayoutBinding bindings[2] = {
            {0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr},
            {1,
             VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
             1,
             VK_SHADER_STAGE_COMPUTE_BIT,
             nullptr}};
        VkDescriptorSetLayoutCreateInfo li{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
        li.bindingCount = 2;
        li.pBindings = bindings;
        vkCheck(vkCreateDescriptorSetLayout(c.device, &li, nullptr, &layout));
        VkDescriptorPoolSize sizes[2] = {{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1},
                                         {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1}};
        VkDescriptorPoolCreateInfo pi{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
        pi.maxSets = 1;
        pi.poolSizeCount = 2;
        pi.pPoolSizes = sizes;
        vkCheck(vkCreateDescriptorPool(c.device, &pi, nullptr, &pool));
        VkDescriptorSetAllocateInfo ai{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
        ai.descriptorPool = pool;
        ai.descriptorSetCount = 1;
        ai.pSetLayouts = &layout;
        vkCheck(vkAllocateDescriptorSets(c.device, &ai, &set));
        VkDescriptorBufferInfo infos[2] = {{output.buffer, 0, output.size},
```

### 同主题跨 API 导航

| 路线 | 程序 |
| --- | --- |
| Vulkan 原生 | [vk14_constant_alignment](README.md) |
| 对照后端 | [dx14_constant_alignment](../../../../DirectX12/lessons/dx14_constant_alignment/README.md) |

### 先预测，再验证

比较请求 257 字节、返回的 allocation size、常量槽 stride 三者，解释不同来源。

参考推导：257 是应用内容大小，allocation size 来自内存分配约束，常量 stride 来自接口偏移对齐。三者可以不同而同时正确。

在 CPU 打印 sizeof/offsetof，再通过 GPU 计算验证第三个常量槽的 factor/bias。

参考推导：CPU 布局断言检查内存，GPU 结果检查接口选择与解码；两份证据互补。第三槽得到 i*7+13，才说明该范围被正确使用。

## 验证与边界

运行 `--smoke` 执行有限自动检查。数值实验读取实际 GPU 结果，图形实验检查非空图像并可查看读回。交互重建与性能还需额外证据；窗口 renderer 当前保留串行等待，双槽计算实验不会被描述为完整多帧呈现引擎。
