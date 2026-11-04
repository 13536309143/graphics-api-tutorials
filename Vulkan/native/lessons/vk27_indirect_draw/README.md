# vk27_indirect_draw · 间接绘制

[54 课目录](../../COURSE.md)

## 目的与实际执行

VkDrawIndirectCommand 的四个 uint 字段与 stride 匹配，缓冲包含 INDIRECT_BUFFER usage，vkCmdDrawIndirect 读取参数。若以后 compute 写参数，需要 compute-write→draw-indirect-read 的依赖。

本程序有独立实验选择与验证路径，公共实现集中在 advanced_labs.cpp。设备/API 检查、数值 compute 与图形渲染采用不同证据；不是每个程序都会弹出窗口。

## 构建和运行

```powershell
cmake --preset vulkan-native
cmake --build --preset vulkan-native-debug --target vk27_indirect_draw --parallel 4
.\Vulkan\native\build\bin\Debug\vk27_indirect_draw.exe
```

## 深入学习：数据、接口与执行

- [第 51 课：间接参数、命令签名与可扩展提交](../../chapters/51_indirect/README.md)

### 本课的真实源码切片

来自 [Vulkan/common/scene_vk.hpp](../../../common/scene_vk.hpp#L434)，第 434—477 行；这是现有实现的局部摘录，末尾可能处于函数中间，不是可单独编译的完整程序。

```cpp
            d.settings.z = std::clamp(d.settings.z * roughnessScale, .045f, 1.f);
            vkCmdPushConstants(cmd,
                               pipelineLayout,
                               VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                               0,
                               sizeof(d),
                               &d);
            if (indirectArguments)
                vkCmdDrawIndirect(cmd, indirectArguments, 0, 1, sizeof(VkDrawIndirectCommand));
            else if (mode == 3)
                vkCmdDraw(cmd, 3, 1, 0, 0);
            else if (mode == 4)
                vkCmdDraw(cmd, uint32_t(mesh.vertices.size()), 1, 0, 0);
            else
                vkCmdDrawIndexed(cmd, uint32_t(mesh.indices.size()), mode == 7 ? 25 : 1, 0, 0, 0);
        }
    }
    ~SceneVk() {
        if (pipeline)
            vkDestroyPipeline(c.device, pipeline, nullptr);
        if (pipelineLayout)
            vkDestroyPipelineLayout(c.device, pipelineLayout, nullptr);
        if (descriptorPool)
            vkDestroyDescriptorPool(c.device, descriptorPool, nullptr);
        if (setLayout)
            vkDestroyDescriptorSetLayout(c.device, setLayout, nullptr);
        if (sampler)
            vkDestroySampler(c.device, sampler, nullptr);
    }
};

// 计算课通过 storage buffer 写入 256 个结果，再在 CPU 上逐项核对。
inline void computeSquares(const VkContextRef& c, const std::filesystem::path& shaders) {
    VkBufferResource storage;
    storage.init(c, 256 * 4, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
    VkDescriptorSetLayoutBinding binding{
        0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr};
    VkDescriptorSetLayoutCreateInfo li{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    li.bindingCount = 1;
    li.pBindings = &binding;
    VkDescriptorSetLayout layout;
    vkCheck(vkCreateDescriptorSetLayout(c.device, &li, nullptr, &layout));
    VkPipelineLayoutCreateInfo pl{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    pl.setLayoutCount = 1;
```

### 同主题跨 API 导航

| 路线 | 程序 |
| --- | --- |
| Vulkan 原生 | [vk27_indirect_draw](README.md) |
| 对照后端 | [dx27_indirect_draw](../../../../DirectX12/lessons/dx27_indirect_draw/README.md) |

### 先预测，再验证

把 vertex count 改为零，预测为何应该没有三角形，并说明它与缓冲为空的区别。

参考推导：合法参数要求零工作，与资源或绑定无效不是一回事。恢复 count 后应重新出现；错误句柄不应该被当作“禁用绘制”手段。

## 验证与边界

运行 `--smoke` 执行有限自动检查。数值实验读取实际 GPU 结果，图形实验检查非空图像并可查看读回。交互重建与性能还需额外证据；窗口 renderer 当前保留串行等待，双槽计算实验不会被描述为完整多帧呈现引擎。

## 实际画面

![vk27_indirect_draw](../../../../docs/previews/vk27_indirect_draw.png)
