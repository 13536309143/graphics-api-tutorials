# 第 12 课 · 图形管线的组合与兼容性 · Vulkan 原生

[54 课目录](../../COURSE.md) · [对照精讲](../../../../docs/compare/02_triangle_pipeline.md) · [上一课](../11_image_views/README.md) · [下一课](../13_shader_modules/README.md)

## 本课目标与先修

学习目标：解释“图形管线的组合与兼容性”解决的问题，找到实际对象和参数，运行对应实验并用数据/画面证明。先完成前一课，并能说明它交付的对象与完成条件；本课在该依赖上增加新契约。

## 为什么需要这一层

管线将可编程阶段与固定功能状态组合，使一次 draw 有明确解释。不要把“shader 编译成功”当成整个管线有效；输入布局、输出目标、深度和写掩码都可能改变最终结果。

## Vulkan 原生 的对象、参数与执行推导

GraphicsPipelineCreateInfo 组合阶段、顶点输入、装配、viewport、raster、multisample、depth、blend、layout 和附件格式。dynamic state 只有声明后才能由相应命令设置。当前三角形用 gl_VertexIndex，故意不引入 VBO。

## 对照 DirectX 12：相同问题的另一种分工

PSO 包含 VS/PS、input layout、raster/blend/depth、目标格式和 sample desc，root signature 提供资源接口。SV_VertexID 路径帮助先验证管线，再在顶点课增加输入资源。

## 关键参数与可检查不变量

| 检查项 | Vulkan 表达 | DX12 表达 | 不变量 / 证据 |
| --- | --- | --- | --- |
| 执行阶段 | VS/FS modules + pipeline | VS/PS bytecode + PSO | 单阶段编译成功不证明整条管线可用 |
| 目标兼容 | PipelineRenderingCreateInfo | RTVFormats / DSVFormat | 附件格式与样本数匹配 |
| 最小三角形 | gl_VertexIndex | SV_VertexID | 此阶段先不引入顶点上传问题 |

先为每项指出源码中的创建/绑定位置，以及证明它成立的实际日志、字节或图像。数值中的设备限制以查询为准；固定实验尺寸不当作所有设备的上限。

## nvpro_core2 框架对照

框架提供命令与目标，不替你决定 SceneVk 的 shader、PSO 组合和格式兼容。onRender 仍需正确绑定和 draw。

这部分是原生 Vulkan 各课的框架对照，框架可运行入口仍为原有六课。如果本节需要新接入工作，会按正文说明；不把原生实验当作已完成的框架实现。

## 运行与实际检查

| 路线 | 实际实验 |
| --- | --- |
| Vulkan 原生 | [vk03_triangle](../../lessons/vk03_triangle/README.md) |
| DirectX 12 | [dx03_triangle](../../../../DirectX12/lessons/dx03_triangle/README.md) |

从合集根目录执行；普通图形程序 Esc 退出，设备/计算/诊断程序输出结果后结束。从“开始学习.cmd”进入本页时，表格可直接点击运行。

```powershell
cmake --preset vulkan-native
cmake --build --preset vulkan-native-debug --target vk03_triangle --parallel 4
.\Vulkan\native\build\bin\Debug\vk03_triangle.exe
ctest --preset vulkan-native-debug -R "^vk03_triangle$" --output-on-failure
```

检查三个层次：创建/绑定/提交的接口约束；数据读回或目标画面；资源使用结束后的回收。诊断实验用合法信息或查询证明渠道可用，不故意保留 API 错误。图形 smoke 的像素检查与数值实验的逐项检查不同，具体实现见源码。

## 关键实现：从当前源码定位

来自 [Vulkan/common/scene_vk.hpp](../../../common/scene_vk.hpp#L391)，第 391—434 行。下列是当前仓库的局部切片，可能在函数中间结束；完整上下文请打开文件，切片不作为单独可编译程序。

```cpp
        VkGraphicsPipelineCreateInfo gp{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
        gp.pNext = &rendering;
        gp.stageCount = 2;
        gp.pStages = stages;
        gp.pVertexInputState = &vi;
        gp.pInputAssemblyState = &ia;
        gp.pViewportState = &viewport;
        gp.pRasterizationState = &raster;
        gp.pMultisampleState = &ms;
        gp.pDepthStencilState = &depth;
        gp.pColorBlendState = &blend;
        gp.pDynamicState = &dynamic;
        gp.layout = pipelineLayout;
        VkResult result = vkCreateGraphicsPipelines(c.device, cache, 1, &gp, nullptr, &pipeline);
        vkDestroyShaderModule(c.device, vs, nullptr);
        vkDestroyShaderModule(c.device, ps, nullptr);
        vkCheck(result, "Graphics pipeline");
    }

    // 这里只录制绘制指令；外层调用者负责开始/结束渲染、附件布局和提交同步。
    void render(VkCommandBuffer cmd, VkExtent2D size, float time, float roughnessScale = 1) {
        auto f = overrideFrame.value_or(frame(mode, float(size.width) / size.height, true));
        if (frameProvider)
            f = frameProvider(float(size.width) / size.height, time, true);
        uniform.upload(&f, sizeof(f));
        VkViewport viewport{
            0, 0, float(size.width) / (halfViewport ? 2 : 1), float(size.height), 0, 1};
        VkRect2D scissor{{0, 0}, size};
        if (halfScissor)
            scissor.extent.width /= 2;
        vkCmdSetViewport(cmd, 0, 1, &viewport);
        vkCmdSetScissor(cmd, 0, 1, &scissor);
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
        vkCmdBindDescriptorSets(
            cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptor, 0, nullptr);
        if (mode >= 4) {
            VkBuffer vb = overrideVertices ? overrideVertices : vertices.buffer;
            VkDeviceSize offset = 0;
            vkCmdBindVertexBuffers(cmd, 0, 1, &vb, &offset);
            if (indices.buffer)
                vkCmdBindIndexBuffer(cmd, indices.buffer, 0, VK_INDEX_TYPE_UINT32);
        }
        for (auto d : overrideDraws.empty() ? draws(mode, time) : overrideDraws) {
            d.settings.z = std::clamp(d.settings.z * roughnessScale, .045f, 1.f);
```

阅读时按“前置对象 → 参数/布局 → 录制/调用 → 完成/可见性 → 释放”五步标注。共享 helper 减少重复代码，但不会取消每课的 API 契约。

## 受控修改实验

将片元/像素输出改为固定洋红，写出黑屏排查的最短路径。

先记录预测，再只改一个因素；保留基线与修改后的输出。不要同时改变 shader、资源范围和同步，这样即使结果变好也无法归因。

### 参考推导与通过标准

先查 draw 是否提交，再查顶点裁剪坐标、viewport/scissor、剔除、深度与目标。固定色仍不出现时先不要研究纹理参数。

## 常见失败与排查顺序

动态渲染不是“动态创建所有管线状态”；附件格式和多样本配置仍须与管线兼容。

先看第一条错误和当前执行路径，再看实际数据/绑定，最后看 shader 与画面。需要 GPU 证据时抓取正常交互帧；仅四帧的 smoke 适合自动检查，不适合手动抓帧。

## 自测与本课交付

能否不看源码回答：将片元/像素输出改为固定洋红，写出黑屏排查的最短路径。

提交一份简短记录：输入与假设、唯一修改、预期、实际证据、原因、是否恢复基线。如果只能复述函数名而说不清何时允许访问/释放，先回到本课的数据流。

## 官方阅读位置与当前工程差异

[指定 Vulkan 教程对应位置](https://docs.vulkan.net.cn/tutorial/latest/03_Drawing_a_triangle/02_Graphics_pipeline_basics/00_Introduction.html)。DX12 路线按同一概念顺序建立映射，其对象与约束以 Microsoft 文档和本后端代码为准。49—54 课包含在基础/compute 之后加入的工程扩展。

[版本与资料说明](../../../../docs/REFERENCES.md) · [调试](../../../../docs/DEBUGGING.md) · [功能完成度](../../../../docs/FEATURE_MATRIX.md)

## 固定输入的实际 GPU 读回

![vk03_triangle](../../../../docs/previews/vk03_triangle.png)
