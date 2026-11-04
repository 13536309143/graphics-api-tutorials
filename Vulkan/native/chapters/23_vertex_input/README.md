# 第 23 课 · 顶点语义、Stride 与 Offset · Vulkan 原生

[54 课目录](../../COURSE.md) · [对照精讲](../../../../docs/compare/03_geometry_memory.md) · [上一课](../22_swapchain_recreation/README.md) · [下一课](../24_vertex_buffer/README.md)

## 本课目标与先修

学习目标：解释“顶点语义、Stride 与 Offset”解决的问题，找到实际对象和参数，运行对应实验并用数据/画面证明。先完成前一课，并能说明它交付的对象与完成条件；本课在该依赖上增加新契约。

## 为什么需要这一层

把 CPU 顶点按字节画出来：position 三 float、normal/颜色三 float、UV 两 float，共 32 字节。stride 决定下一顶点起点，offset 决定本顶点属性位置。正确地址与正确解释是两项条件。

## Vulkan 原生 的对象、参数与执行推导

binding 描述 stride 与每顶点/每实例速率，attribute 描述 location、format、offset。本例位置/颜色/UV 偏移 0/12/24。输入 location 不是 descriptor binding，二者服务不同路径。

## 对照 DirectX 12：相同问题的另一种分工

VBV 给 GPU 地址、大小和 stride，input layout 用 POSITION/NORMAL/TEXCOORD 连接 HLSL 输入。语义名称与偏移分别检查，不能因为名字相同就忽略格式。实例数据还要声明独立的输入分类/步进。

## 关键参数与可检查不变量

| 检查项 | Vulkan 表达 | DX12 表达 | 不变量 / 证据 |
| --- | --- | --- | --- |
| 顶点尺寸 | Vertex=32 bytes | Vertex=32 bytes | position/normal/UV 连续契约 |
| 属性偏移 | 0/12/24 | 0/12/24 | format 与 stride 一起正确 |
| 输入编号 | attribute location | semantic + input layout | 与 descriptor/register 是不同系统 |

先为每项指出源码中的创建/绑定位置，以及证明它成立的实际日志、字节或图像。数值中的设备限制以查询为准；固定实验尺寸不当作所有设备的上限。

## nvpro_core2 框架对照

VMA 改变分配方式，不改变 Vertex 的字节布局。把 GPU storage buffer 同时当顶点缓冲时，更需要核对两种接口的 stride。

这部分是原生 Vulkan 各课的框架对照，框架可运行入口仍为原有六课。如果本节需要新接入工作，会按正文说明；不把原生实验当作已完成的框架实现。

## 运行与实际检查

| 路线 | 实际实验 |
| --- | --- |
| Vulkan 原生 | [vk53_vertex_layout](../../lessons/vk53_vertex_layout/README.md) |
| DirectX 12 | [dx53_vertex_layout](../../../../DirectX12/lessons/dx53_vertex_layout/README.md) |

从合集根目录执行；普通图形程序 Esc 退出，设备/计算/诊断程序输出结果后结束。从“开始学习.cmd”进入本页时，表格可直接点击运行。

```powershell
cmake --preset vulkan-native
cmake --build --preset vulkan-native-debug --target vk53_vertex_layout --parallel 4
.\Vulkan\native\build\bin\Debug\vk53_vertex_layout.exe
ctest --preset vulkan-native-debug -R "^vk53_vertex_layout$" --output-on-failure
```

检查三个层次：创建/绑定/提交的接口约束；数据读回或目标画面；资源使用结束后的回收。诊断实验用合法信息或查询证明渠道可用，不故意保留 API 错误。图形 smoke 的像素检查与数值实验的逐项检查不同，具体实现见源码。

## 关键实现：从当前源码定位

来自 [Vulkan/common/scene_vk.hpp](../../../common/scene_vk.hpp#L343)，第 343—386 行。下列是当前仓库的局部切片，可能在函数中间结束；完整上下文请打开文件，切片不作为单独可编译程序。

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

阅读时按“前置对象 → 参数/布局 → 录制/调用 → 完成/可见性 → 释放”五步标注。共享 helper 减少重复代码，但不会取消每课的 API 契约。

## 受控修改实验

把某一个属性偏移故意改错，先预测影响的是形状、颜色还是 UV，再用 mesh viewer 查字节。

先记录预测，再只改一个因素；保留基线与修改后的输出。不要同时改变 shader、资源范围和同步，这样即使结果变好也无法归因。

### 参考推导与通过标准

位置错会改变几何，颜色/法线错影响插值或光照，UV 错影响采样。一次只改一项才能判断其责任。

## 常见失败与排查顺序

导入模型扩展 joints/weights 后仍套用 32 字节布局，或把整数 joint 当浮点 attribute，会产生完全错误的输入。

先看第一条错误和当前执行路径，再看实际数据/绑定，最后看 shader 与画面。需要 GPU 证据时抓取正常交互帧；仅四帧的 smoke 适合自动检查，不适合手动抓帧。

## 自测与本课交付

能否不看源码回答：把某一个属性偏移故意改错，先预测影响的是形状、颜色还是 UV，再用 mesh viewer 查字节。

提交一份简短记录：输入与假设、唯一修改、预期、实际证据、原因、是否恢复基线。如果只能复述函数名而说不清何时允许访问/释放，先回到本课的数据流。

## 官方阅读位置与当前工程差异

[指定 Vulkan 教程对应位置](https://docs.vulkan.net.cn/tutorial/latest/04_Vertex_buffers/00_Vertex_input_description.html)。DX12 路线按同一概念顺序建立映射，其对象与约束以 Microsoft 文档和本后端代码为准。49—54 课包含在基础/compute 之后加入的工程扩展。

[版本与资料说明](../../../../docs/REFERENCES.md) · [调试](../../../../docs/DEBUGGING.md) · [功能完成度](../../../../docs/FEATURE_MATRIX.md)

## 固定输入的实际 GPU 读回

![vk53_vertex_layout](../../../../docs/previews/vk53_vertex_layout.png)
