# vk53_vertex_layout · 顶点布局检验

[54 课目录](../../COURSE.md)

## 目的与实际执行

binding 描述 stride 与每顶点/每实例速率，attribute 描述 location、format、offset。本例位置/颜色/UV 偏移 0/12/24。输入 location 不是 descriptor binding，二者服务不同路径。

本程序有独立实验选择与验证路径，公共实现集中在 advanced_labs.cpp。设备/API 检查、数值 compute 与图形渲染采用不同证据；不是每个程序都会弹出窗口。

## 构建和运行

```powershell
cmake --preset vulkan-native
cmake --build --preset vulkan-native-debug --target vk53_vertex_layout --parallel 4
.\Vulkan\native\build\bin\Debug\vk53_vertex_layout.exe
```

## 深入学习：数据、接口与执行

- [第 23 课：顶点语义、Stride 与 Offset](../../chapters/23_vertex_input/README.md)

### 本课的真实源码切片

来自 [Vulkan/common/scene_vk.hpp](../../../common/scene_vk.hpp#L343)，第 343—386 行；这是现有实现的局部摘录，末尾可能处于函数中间，不是可单独编译的完整程序。

```cpp
        VkVertexInputBindingDescription bind{0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX};
        VkVertexInputAttributeDescription attributes[3] = {{0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0},
                                                           {1, 0, VK_FORMAT_R32G32B32_SFLOAT, 12},
                                                           {2, 0, VK_FORMAT_R32G32_SFLOAT, 24}};
        VkPipelineVertexInputStateCreateInfo vi{
            VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
        if (mode >= 4) {
            vi.vertexBindingDescriptionCount = 1;
            vi.pVertexBindingDescriptions = &bind;
            vi.vertexAttributeDescriptionCount = 3;
            vi.pVertexAttributeDescriptions = attributes;
        }
        VkPipelineInputAssemblyStateCreateInfo ia{
            VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
        ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        VkPipelineViewportStateCreateInfo viewport{
            VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
        viewport.viewportCount = viewport.scissorCount = 1;
        VkPipelineRasterizationStateCreateInfo raster{
            VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
        raster.polygonMode = VK_POLYGON_MODE_FILL;
        raster.cullMode = cull;
        raster.lineWidth = 1;
        VkPipelineMultisampleStateCreateInfo ms{
            VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
        ms.rasterizationSamples = samples;
        VkPipelineDepthStencilStateCreateInfo depth{
            VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
        depth.depthTestEnable = depth.depthWriteEnable = mode >= 5;
        depth.depthCompareOp = VK_COMPARE_OP_LESS;
        VkPipelineColorBlendAttachmentState attachment{};
        attachment.colorWriteMask =
            redOnly ? VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_A_BIT : 15;
        VkPipelineColorBlendStateCreateInfo blend{
            VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
        blend.attachmentCount = 1;
        blend.pAttachments = &attachment;
        VkDynamicState ds[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
        VkPipelineDynamicStateCreateInfo dynamic{
            VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
        dynamic.dynamicStateCount = 2;
        dynamic.pDynamicStates = ds;

        // 动态渲染不创建 render pass，但管线仍需声明颜色和深度附件格式。
```

### 同主题跨 API 导航

| 路线 | 程序 |
| --- | --- |
| Vulkan 原生 | [vk53_vertex_layout](README.md) |
| 对照后端 | [dx53_vertex_layout](../../../../DirectX12/lessons/dx53_vertex_layout/README.md) |

### 先预测，再验证

把某一个属性偏移故意改错，先预测影响的是形状、颜色还是 UV，再用 mesh viewer 查字节。

参考推导：位置错会改变几何，颜色/法线错影响插值或光照，UV 错影响采样。一次只改一项才能判断其责任。

## 验证与边界

运行 `--smoke` 执行有限自动检查。数值实验读取实际 GPU 结果，图形实验检查非空图像并可查看读回。交互重建与性能还需额外证据；窗口 renderer 当前保留串行等待，双槽计算实验不会被描述为完整多帧呈现引擎。

## 实际画面

![vk53_vertex_layout](../../../../docs/previews/vk53_vertex_layout.png)
