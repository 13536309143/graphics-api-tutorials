# vk42_shader_inspection · 着色器产物检查

[54 课目录](../../COURSE.md)

## 目的与实际执行

glslc 生成 SPIR-V，模块创建要求长度与 word 对齐等合法条件。实验检查 magic、word 数和 module 创建。SPIR-V 的 ID bound 不是线程数量，模块也不决定最终 render target。

本程序有独立实验选择与验证路径，公共实现集中在 advanced_labs.cpp。设备/API 检查、数值 compute 与图形渲染采用不同证据；不是每个程序都会弹出窗口。

## 构建和运行

```powershell
cmake --preset vulkan-native
cmake --build --preset vulkan-native-debug --target vk42_shader_inspection --parallel 4
.\Vulkan\native\build\bin\Debug\vk42_shader_inspection.exe
```

## 深入学习：数据、接口与执行

- [第 13 课：着色器编译、模块与反射](../../chapters/13_shader_modules/README.md)

### 本课的真实源码切片

来自 [Vulkan/native/common/advanced_labs.cpp](../../common/advanced_labs.cpp#L835)，第 835—878 行；这是现有实现的局部摘录，末尾可能处于函数中间，不是可单独编译的完整程序。

```cpp
        for (uint32_t i = 0; i < count; ++i)
            std::cout << "family=" << i << ", queues=" << queues[i].queueCount
                      << ", flags=" << queues[i].queueFlags
                      << ", timestampBits=" << queues[i].timestampValidBits << '\n';
        require((queues[c.family].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0,
                "Selected family lacks graphics");
    } else if (id == 42) {
        auto data = spirv(nativeShaders / "lab.comp.spv");
        require(data[0] == 0x07230203, "Wrong SPIR-V magic");
        auto module = shaderModule(c.device, nativeShaders / "lab.comp.spv");
        vkDestroyShaderModule(c.device, module, nullptr);
        std::cout << "Verified SPIR-V module, words=" << data.size() << ", bound=" << data[3]
                  << '\n';
    } else if (id == 44)
        commandReuse(c);
    else if (id == 45) {
        ComputeKernel kernel(c);
        kernel.execute(0, 128, 2, 3);
        auto first = readWords(kernel.output, 128);
        VkBufferResource second;
        second.init(c, 4096, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
        VkDescriptorBufferInfo info{second.buffer, 0, second.size};
        VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
        write.dstSet = kernel.set;
        write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        write.descriptorCount = 1;
        write.pBufferInfo = &info;
        vkUpdateDescriptorSets(c.device, 1, &write, 0, nullptr);
        immediate(c, [&](VkCommandBuffer cmd) {
            kernel.record(cmd, 0, 128, 5, 11);
            bufferDependency(cmd,
                             second.buffer,
                             VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                             VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
                             VK_PIPELINE_STAGE_2_HOST_BIT,
                             VK_ACCESS_2_HOST_READ_BIT);
        });
        auto changed = readWords(second, 128);
        require(readWords(kernel.output, 128) == first, "Descriptor update overwrote prior buffer");
        for (uint32_t i = 0; i < 128; ++i)
            require(changed[i] == i * 5 + 11, "Updated descriptor points at wrong storage");
        std::cout << "Verified descriptor replacement after prior submission completion\n";
    } else if (id == 46) {
        for (auto format :
```

### 同主题跨 API 导航

| 路线 | 程序 |
| --- | --- |
| Vulkan 原生 | [vk42_shader_inspection](README.md) |
| 对照后端 | [dx42_shader_inspection](../../../../DirectX12/lessons/dx42_shader_inspection/README.md) |

### 先预测，再验证

将 shader 的 local size 改为 32，但 host 的组数不改，预测覆盖范围和验证结果。

参考推导：总调用数量减半，尾部不会按原契约写入。反射/声明、dispatch 组数、缓冲容量和 CPU 检查必须共同更新。

## 验证与边界

运行 `--smoke` 执行有限自动检查。数值实验读取实际 GPU 结果，图形实验检查非空图像并可查看读回。交互重建与性能还需额外证据；窗口 renderer 当前保留串行等待，双槽计算实验不会被描述为完整多帧呈现引擎。
