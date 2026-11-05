# 05 · Uniform、Descriptor、CBV 与根签名

[对照路线](../COMPARE.md) · [交互式对照](index.html#05_binding_constants) · [上一章](04_transforms_camera.md) · [下一章](06_textures_color.md)

本章回答“shader 要一个矩阵，为什么 Vulkan/DX12 需要布局、池、集合或根参数”。先把接口约定和实际数据分开，就不会把更新内存与更新绑定混淆。

## 用频率划分数据

顶点数据按顶点变化；相机矩阵按帧变化；材质纹理按材质变化；model 矩阵按 draw 变化。频率划分决定资源如何复用，不由 API 函数名替你决定。为每个 draw 重建所有 descriptor，或把整个场景塞进 push constants/root constants，都会让后续扩展困难。

本仓库共享 `Frame` 只有一个 64 字节 viewProjection。`Draw` 为 64 字节 model 加 16 字节 settings，总计 80 字节。settings 的 x/y/z/w 对应模式、金属度、粗糙度、时间。CPU 的 static_assert、shader 的声明和实际绑定范围共同构成契约。

| 层次 | OpenGL | Vulkan | DX12 |
| --- | --- | --- | --- |
| 声明接口 | uniform 名称 / uniform block | set layout / pipeline layout | root signature |
| 保存数据 | uniform 状态或 UBO | UBO / texture / storage buffer | 常量或其他 resource |
| 选择实际资源 | texture unit、buffer binding point | descriptor set 的 write/update | descriptor heap、root descriptor |
| 每 draw 小数据 | glUniform | push constants | root constants |

## 按本代码拆解一次绑定

Vulkan 先创建两个 layout binding：0 是 vertex 阶段 UBO，1 是 fragment 阶段组合图像采样器；从 pool 分配 set，再更新其 buffer/image 引用。渲染时绑定 pipeline、set，逐对象推送 Draw。set layout 是“接口形状”，set 是“填好的引用”，buffer 才存矩阵字节。`vkUpdateDescriptorSets` 不会把 buffer 内容复制进集合。

DX12 用 root parameter 0 直接传 b0 的 GPU 地址，parameter 1 是 b1 对应的 20 个 DWORD，parameter 2 是 t0 的 SRV descriptor table。root parameter 的数组下标与 HLSL register 编号不是一套编号：parameter 2 可以指向 t0，而不是必须 t2。当前 sampler 采用 static sampler，示例只需要可见 CBV/SRV/UAV heap。

OpenGL 的普通 uniform 属于 program，UBO 则通过 block index、block binding 和 buffer binding point 联系起来。`glGetUniformBlockIndex` 得到的是程序内部块索引，不是 `glBindBufferBase` 的绑定点。OpenGL 28 正好用于观察这三个层次。

## 对齐不只是 sizeof

DX12 常量缓冲 GPU 地址要求 256 字节对齐，本课程为 Frame 分配 256 字节。Vulkan 的动态 UBO offset 要按设备的 minUniformBufferOffsetAlignment 处理；OpenGL buffer range 也有相应对齐限制。当前 Vulkan 使用普通 UBO，并未实现 dynamic offset。`alignas(16)` 保证部分 C++ 布局需求，不代表自动满足所有 API 偏移约束。

## 实验：增加第二个对象

复用一份 Frame，增加两个 Draw，各自设置 model 和粗糙度。预期：相机变化影响两者，修改其中一个对象的 Draw 只影响该对象。不要为每个对象重建 pipeline。抓帧核对两次 draw 的 push/root constants，而不是只在 CPU 断点看最后一个变量值。

自测：为什么 descriptor 指向正确 buffer，画面仍会使用错误矩阵？答案可能是字节布局不符、矩阵语义不符，或 CPU 在 GPU 仍读取时覆盖了同一份内存。绑定正确不能代替数据与生命周期正确。

规则入口：[Vulkan dynamic offset](https://docs.vulkan.org/guide/latest/descriptor_dynamic_offset.html)、[Microsoft root signatures](https://learn.microsoft.com/en-us/windows/win32/direct3d12/root-signatures)。本章的字段和大小以本仓库源码为准。

<!-- generated-source-comparison -->

## 与本仓库实现并排核对

以下切片直接取自当前仓库，省略了前后文。行号用于定位，不能把片段当作完整可编译程序。

| 路线 | 可运行参照 / 范围 |
| --- | --- |
| OpenGL | [28_ubo](../../OpenGL/lessons/28_ubo/README.md) |
| Vulkan 原生 | [vk05_indexed_uniform](../../Vulkan/native/lessons/vk05_indexed_uniform/README.md) |
| Vulkan / nvpro_core2 | [nv06_pbr_viewport](../../Vulkan/nvpro/lessons/nv06_pbr_viewport/README.md) |
| DirectX 12 | [dx05_indexed_cbv](../../DirectX12/lessons/dx05_indexed_cbv/README.md) |

### OpenGL · 已实现

block index、binding point 与实际 buffer 三层分开检查。

[OpenGL/lessons/28_ubo/main.cpp](../../OpenGL/lessons/28_ubo/main.cpp#L19)，第 19—45 行：

```cpp
    GLuint block = glGetUniformBlockIndex(shader.id, "CameraBlock");
    if (block == GL_INVALID_INDEX)
        throw std::runtime_error("Missing uniform block");
    glUniformBlockBinding(shader.id, block, 0);
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, buffer);
    // 每帧处理输入并绘制；GPU 对象在退出循环后按 C++ 作用域逆序释放。
    while (app.begin()) {
        camera.update(app);
        app.clear();
        auto projection = camera.projection(app), view = camera.view();
        glBindBuffer(GL_UNIFORM_BUFFER, buffer);
        glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4), glm::value_ptr(projection));
        glBufferSubData(
            GL_UNIFORM_BUFFER, sizeof(glm::mat4), sizeof(glm::mat4), glm::value_ptr(view));
        shader.use();
        shader.set("opacity", 1.f);
        for (int n = 0; n < 5; ++n) {
            auto model = glm::translate(glm::mat4(1), glm::vec3((n - 2) * 1.3f, 0, 0));
            shader.set("model", glm::rotate(model, app.time() * .3f + n, glm::vec3(.2f, 1, .1f)));
            shader.set("baseColor", glm::vec3(.15f * n, .6f, 1 - .15f * n));
            box.draw();
        }
        app.end();
    }
    glDeleteBuffers(1, &buffer);
    return 0;
} catch (const std::exception& e) {
```

### Vulkan 原生 · 已实现

layout 规定类型/阶段，set 填资源引用，buffer 才保存矩阵。

[Vulkan/native/lessons/vk05_indexed_uniform/support/scene_vk.hpp](../../Vulkan/native/lessons/vk05_indexed_uniform/support/scene_vk.hpp#L284)，第 284—321 行：

```cpp
        VkDescriptorSetLayoutBinding bindings[2] = {
            {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr},
            {1,
             VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
             1,
             VK_SHADER_STAGE_FRAGMENT_BIT,
             nullptr}};
        VkDescriptorSetLayoutCreateInfo li{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
        li.bindingCount = 2;
        li.pBindings = bindings;
        // 绑定契约：set/binding、描述符类型和 shader 可见阶段必须与 GLSL 一致。
        vkCheck(vkCreateDescriptorSetLayout(c.device, &li, nullptr, &setLayout));
        VkDescriptorPoolSize sizes[2] = {{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1},
                                         {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1}};
        VkDescriptorPoolCreateInfo pool{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
        pool.maxSets = 1;
        pool.poolSizeCount = 2;
        pool.pPoolSizes = sizes;
        vkCheck(vkCreateDescriptorPool(c.device, &pool, nullptr, &descriptorPool));
        VkDescriptorSetAllocateInfo da{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
        da.descriptorPool = descriptorPool;
        da.descriptorSetCount = 1;
        da.pSetLayouts = &setLayout;
        vkCheck(vkAllocateDescriptorSets(c.device, &da, &descriptor));
        VkDescriptorBufferInfo ub{uniform.buffer, 0, sizeof(Frame)};
        VkDescriptorImageInfo image{
            sampler, texture.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
        VkWriteDescriptorSet writes[2]{};
        for (auto& w : writes) {
            w.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            w.dstSet = descriptor;
            w.descriptorCount = 1;
        }
        writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        writes[0].pBufferInfo = &ub;
        writes[1].dstBinding = 1;
        writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writes[1].pImageInfo = &image;
```

### Vulkan / nvpro_core2 · 已实现

SceneVk 逐 draw 更新 push constants；框架只提供外围上下文。

[Vulkan/nvpro/lessons/nv06_pbr_viewport/support/scene_vk.hpp](../../Vulkan/nvpro/lessons/nv06_pbr_viewport/support/scene_vk.hpp#L438)，第 438—456 行：

```cpp
            vkCmdPushConstants(cmd,
                               pipelineLayout,
                               VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                               0,
                               sizeof(d),
                               &d);
            if (indirectArguments)
                // 绘制消费此前绑定的管线、顶点与资源；布局和资源状态必须已满足。
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
```

### DirectX 12 · 已实现

root parameter 2 指 t0，0 指 b0；两种编号不能混同。

[DirectX12/lessons/dx05_indexed_cbv/support/dx12_course.hpp](../../DirectX12/lessons/dx05_indexed_cbv/support/dx12_course.hpp#L353)，第 353—388 行：

```cpp
        D3D12_ROOT_PARAMETER params[3]{};
        params[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        params[0].Descriptor.ShaderRegister = 0;
        params[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
        params[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
        params[1].Constants = {1, 0, 20};
        params[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        D3D12_DESCRIPTOR_RANGE range{};
        range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        range.NumDescriptors = 1;
        range.BaseShaderRegister = 0;
        range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
        params[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        params[2].DescriptorTable = {1, &range};
        params[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
        D3D12_STATIC_SAMPLER_DESC sampler{};
        sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        sampler.AddressU = sampler.AddressV = sampler.AddressW =
            clampSampler ? D3D12_TEXTURE_ADDRESS_MODE_CLAMP : D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        sampler.MaxLOD = D3D12_FLOAT32_MAX;
        sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
        sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
        D3D12_ROOT_SIGNATURE_DESC root{};
        root.NumParameters = 3;
        root.pParameters = params;
        root.NumStaticSamplers = 1;
        root.pStaticSamplers = &sampler;
        root.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
        signature = rootSignature(c, root);
        auto vs = compileHlsl(mode == 3 ? "TriangleMain" : "VertexMain", "vs_5_1"),
             ps = compileHlsl("PixelMain", "ps_5_1");
        D3D12_INPUT_ELEMENT_DESC elements[] = {{"POSITION",
                                                0,
                                                DXGI_FORMAT_R32G32B32_FLOAT,
                                                0,
                                                0,
```
