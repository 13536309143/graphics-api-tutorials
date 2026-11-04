# vk29_gpu_particles · GPU 生成顶点与绘制

[54 课目录](../../COURSE.md)

## 目的与实际执行

同一 buffer 具有 STORAGE_BUFFER 和 VERTEX_BUFFER usage。计算写入通过 compute-storage-write→vertex-attribute-read 依赖连接绘制。scalar 形式的 storage Vertex 与输入 32 字节 stride 一致，避免 std430 vec3 对齐误配。 直接裁剪坐标在绘制的 Frame 里显式翻转 Y，使数学粒子位置与 DX12 视口映射一致；GPU 生成的数据仍保持同一布局。

本程序有独立实验选择与验证路径，公共实现集中在 advanced_labs.cpp。设备/API 检查、数值 compute 与图形渲染采用不同证据；不是每个程序都会弹出窗口。

## 构建和运行

```powershell
cmake --preset vulkan-native
cmake --build --preset vulkan-native-debug --target vk29_gpu_particles --parallel 4
.\Vulkan\native\build\bin\Debug\vk29_gpu_particles.exe
```

## 深入学习：数据、接口与执行

- [第 48 课：计算生成顶点并直接绘制](../../chapters/48_compute_graphics/README.md)

### 本课的真实源码切片

来自 [Vulkan/native/common/advanced_labs.cpp](../../common/advanced_labs.cpp#L536)，第 536—579 行；这是现有实现的局部摘录，末尾可能处于函数中间，不是可单独编译的完整程序。

```cpp
struct ParticleKernel {
    VkContextRef c;
    VkBufferResource vertices;
    VkDescriptorSetLayout layout{};
    VkDescriptorPool pool{};
    VkDescriptorSet set{};
    VkPipelineLayout pipelineLayout{};
    VkPipeline pipeline{};
    explicit ParticleKernel(VkContextRef context) : c(context) {
        vertices.init(c,
                      96 * 3 * sizeof(Vertex),
                      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        VkDescriptorSetLayoutBinding binding{
            0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr};
        VkDescriptorSetLayoutCreateInfo li{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
        li.bindingCount = 1;
        li.pBindings = &binding;
        vkCheck(vkCreateDescriptorSetLayout(c.device, &li, nullptr, &layout));
        VkDescriptorPoolSize size{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1};
        VkDescriptorPoolCreateInfo pi{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
        pi.maxSets = 1;
        pi.poolSizeCount = 1;
        pi.pPoolSizes = &size;
        vkCheck(vkCreateDescriptorPool(c.device, &pi, nullptr, &pool));
        VkDescriptorSetAllocateInfo ai{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
        ai.descriptorPool = pool;
        ai.descriptorSetCount = 1;
        ai.pSetLayouts = &layout;
        vkCheck(vkAllocateDescriptorSets(c.device, &ai, &set));
        VkDescriptorBufferInfo bi{vertices.buffer, 0, vertices.size};
        VkWriteDescriptorSet w{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
        w.dstSet = set;
        w.descriptorCount = 1;
        w.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        w.pBufferInfo = &bi;
        vkUpdateDescriptorSets(c.device, 1, &w, 0, nullptr);
        VkPushConstantRange range{VK_SHADER_STAGE_COMPUTE_BIT, 0, 8};
        VkPipelineLayoutCreateInfo pl{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
        pl.setLayoutCount = 1;
        pl.pSetLayouts = &layout;
        pl.pushConstantRangeCount = 1;
        pl.pPushConstantRanges = &range;
        vkCheck(vkCreatePipelineLayout(c.device, &pl, nullptr, &pipelineLayout));
```

### 同主题跨 API 导航

| 路线 | 程序 |
| --- | --- |
| Vulkan 原生 | [vk29_gpu_particles](README.md) |
| 对照后端 | [dx29_gpu_particles](../../../../DirectX12/lessons/dx29_gpu_particles/README.md) |

### 先预测，再验证

把粒子数改为 97，需要检查哪些容量与 dispatch/guard？

参考推导：顶点数变为 291，GPU buffer 与 CPU 侧 draw count 元数据都要扩大；组数向上取整，shader 以 count*3 为上界。位置仍由 GPU 生成。

## 验证与边界

运行 `--smoke` 执行有限自动检查。数值实验读取实际 GPU 结果，图形实验检查非空图像并可查看读回。交互重建与性能还需额外证据；窗口 renderer 当前保留串行等待，双槽计算实验不会被描述为完整多帧呈现引擎。

## 实际画面

![vk29_gpu_particles](../../../../docs/previews/vk29_gpu_particles.png)
