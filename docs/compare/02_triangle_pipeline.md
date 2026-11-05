# 02 · 从三角形看管线与着色器

[对照路线](../COMPARE.md) · [交互式对照](index.html#02_triangle_pipeline) · [上一章](01_context_device.md) · [下一章](03_geometry_memory.md)

把复杂模型暂时拿掉，只保留一个三角形，可以让“顶点输入错了”和“管线根本没有工作”分开。本章沿输入装配、顶点处理、裁剪、光栅化、片元/像素着色、深度和混合逐段检查。

## 三个顶点为什么变成一片颜色

顶点着色器为每个顶点输出裁剪空间位置和待插值数据。光栅化生成三角形覆盖的片段，并插值颜色、UV 等属性；片元着色器处理的是片段，不是原始的三个顶点。最终颜色还受到深度、模板、混合和颜色写掩码影响，因此“shader 返回红色”不保证屏幕上出现红色。

| 配置 | OpenGL | Vulkan | DX12 |
| --- | --- | --- | --- |
| 着色器代码 | GLSL 源码在运行时编译/链接 | GLSL 由 glslc 生成 SPIR-V | HLSL 在运行时编译为 SM5.1 字节码 |
| 管线对象 | program 与分散的状态调用 | graphics pipeline + pipeline layout | PSO + root signature |
| 绘制命令 | glDrawArrays/Elements | vkCmdDraw/DrawIndexed | DrawInstanced/DrawIndexedInstanced |
| 可动态改变的状态 | 大量 context 状态 | 取决于 pipeline 的 dynamic state 声明 | viewport/scissor 等通过命令设置 |

OpenGL `Shader` 将编译和链接分开检查。编译通过只说明单个阶段语法/类型成立；链接还检查跨阶段接口和程序可执行性。Vulkan shader module 也不等于完整管线，阶段入口、顶点布局、目标格式等在 pipeline 创建时组合。DX12 PSO 将着色器与固定功能状态组合，root signature 再声明访问资源的接口。

## 本仓库的三角形不完全同构

OpenGL 02 使用 VBO 和 VAO。vk03 使用专门的 `triangle.vert`，通过 `gl_VertexIndex` 生成位置和颜色；dx03 通过 `SV_VertexID` 做同样的事。nv03 把同一个底层 Vulkan 三角形输出到框架离屏视口。比较时应承认 OpenGL 多了顶点上传步骤，而不是把初始化行数当成 API 固有成本。

vk03/dx03 采用程序生成顶点，是为了先验证管线和呈现；vk04/dx04 才加入顶点缓冲。公共渲染器预创建了后续课需要的一些资源，阅读最小三角形时可以先追踪入口、管线创建和 draw 三个位置，稍后再展开纹理与 descriptor 初始化。

## 六步定位一个不可见三角形

1. 确认 draw 命令确实进入提交的 command buffer/list，OpenGL 则确认循环和当前 program。
2. 确认输出位置在合理裁剪范围内，先把矩阵设为单位矩阵。
3. 检查 viewport/scissor 的宽高，尤其是窗口最小化后的零尺寸。
4. 暂时检查剔除与 front-face 约定，避免三角形全部被剔掉。
5. 确认颜色写掩码和 render target 格式，不要只看像素 shader。
6. 检查输出到的是交换链目标、离屏纹理还是 UI 最终合成目标。

## 实验：只改变颜色

把片元/像素输出改为固定洋红，重新构建后运行。若变为洋红，顶点到目标的链路基本成立，可以继续查插值或材质；若没有变化，优先确认修改的是源码而不是 exe 旁的副本，以及当前运行的 Debug/Release 是否对应。

再把一个顶点移到三角形外侧，预测面积和颜色插值如何变化。参考答案：顶点数量仍是 3，覆盖片段数量改变；颜色随重心坐标插值，不能用“每顶点只有一种颜色”推断整个三角形只有三种颜色。

## 从参考教程迁移

旧 Vulkan 材料可能创建 render pass/framebuffer，本仓库使用 dynamic rendering。它省去了那组对象，不会省去附件格式、清除策略和同步要求。[官方管线介绍](https://docs.vulkan.org/tutorial/latest/03_Drawing_a_triangle/02_Graphics_pipeline_basics/00_Introduction.html) 适合帮助定位各阶段，但具体字段应对照本项目使用的 API 基线。

<!-- generated-source-comparison -->

## 与本仓库实现并排核对

以下切片直接取自当前仓库，省略了前后文。行号用于定位，不能把片段当作完整可编译程序。

| 路线 | 可运行参照 / 范围 |
| --- | --- |
| OpenGL | [02_triangle](../../OpenGL/lessons/02_triangle/README.md) |
| Vulkan 原生 | [vk03_triangle](../../Vulkan/native/lessons/vk03_triangle/README.md) |
| Vulkan / nvpro_core2 | [nv03_triangle](../../Vulkan/nvpro/lessons/nv03_triangle/README.md) |
| DirectX 12 | [dx03_triangle](../../DirectX12/lessons/dx03_triangle/README.md) |

### OpenGL · 已实现

OpenGL 02 使用实际 VBO，和另两条程序生成顶点的路径有范围差别。

[OpenGL/lessons/02_triangle/main.cpp](../../OpenGL/lessons/02_triangle/main.cpp#L7)，第 7—28 行：

```cpp
int main(int argc, char** argv) try {
    // 先创建上下文；它必须比 shader、纹理和网格活得更久。
    App app(argc, argv, "02_triangle - OpenGL Course");
    // shader 路径指向本课自己的文件；属性、uniform 与下方 C++ 设置对应。
    Shader shader(app, "basic.vert", "color.frag");
    // CPU 顶点数据上传后，由 VAO 保存属性布局；布局必须匹配顶点 shader。
    Mesh mesh(
        {-.7f, -.5f, 0, 1, 0, 0, 0, 0, .7f, -.5f, 0, 0, 1, 0, 1, 0, 0, .6f, 0, 0, 0, 1, .5f, 1});
    // 每帧处理输入并绘制；GPU 对象在退出循环后按 C++ 作用域逆序释放。
    while (app.begin()) {
        app.clear();
        shader.use();
        glm::mat4 mvp(1.f);
        shader.set("mvp", mvp);
        shader.set("pulse", 1.f);
        mesh.draw();
        app.end();
    }
    return 0;
} catch (const std::exception& e) {
    return failure(e);
}
```

### Vulkan 原生 · 已实现

动态渲染仍要求 pipeline 声明附件格式；还要设置 viewport/scissor。

[Vulkan/native/lessons/vk03_triangle/support/scene_vk.hpp](../../Vulkan/native/lessons/vk03_triangle/support/scene_vk.hpp#L394)，第 394—425 行：

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
```

### Vulkan / nvpro_core2 · 已实现

场景写入离屏 RenderTarget，最后才被 ImGui 采样。

[Vulkan/nvpro/lessons/nv03_triangle/support/nvapp_course.hpp](../../Vulkan/nvpro/lessons/nv03_triangle/support/nvapp_course.hpp#L130)，第 130—171 行：

```cpp
    void onRender(VkCommandBuffer cmd) override {
        // 每次更新前等待队列，保证共用 UBO 和离屏目标不会与前一帧并发读写。
        vkCheck(vkQueueWaitIdle(c.queue));

        // GENERAL 布局不代表自动同步：仍需声明前一次读写与本次颜色写入的依赖。
        barrier(cmd,
                target.getColorImage(),
                VK_IMAGE_LAYOUT_GENERAL,
                VK_IMAGE_LAYOUT_GENERAL,
                VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
                VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT,
                VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT);
        barrier(cmd,
                target.getDepthImage(),
                VK_IMAGE_LAYOUT_GENERAL,
                VK_IMAGE_LAYOUT_GENERAL,
                VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
                VK_ACCESS_2_MEMORY_WRITE_BIT,
                VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT |
                    VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
                VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                VK_IMAGE_ASPECT_DEPTH_BIT);
        nvvk::RenderTargetState state;
        target.fillState(state);
        state.colorAttachments[0].clearValue.color = {{.035f, .055f, .09f, 1}};
        state.depthAttachment.clearValue.depthStencil = {1, 0};
        nvvk::RenderTargetState::AttachmentOps ops;
        state.cmdBeginRendering(cmd, ops);
        if (scene)
            scene->render(
                cmd, target.getSize(), smoke ? 1.f : float(ImGui::GetTime()), roughnessScale);
        vkCmdEndRendering(cmd);
        barrier(cmd,
                target.getColorImage(),
                VK_IMAGE_LAYOUT_GENERAL,
                VK_IMAGE_LAYOUT_GENERAL,
                VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
                VK_ACCESS_2_SHADER_SAMPLED_READ_BIT);
    }
```

### DirectX 12 · 已实现

由顶点 ID 生成位置/颜色，先把输入布局问题排除。

[DirectX12/shaders/scene.hlsl](../../DirectX12/shaders/scene.hlsl#L39)，第 39—55 行：

```hlsl
Pixel TriangleMain(uint id : SV_VertexID) {
    float2 points[3] = {float2(-.7, -.6), float2(.7, -.6), float2(0, .6)};
    float3 colors[3] = {float3(1, 0, 0), float3(0, 1, 0), float3(0, 0, 1)};
    Pixel p;
    p.pos = float4(points[id], 0, 1);
    p.normal = colors[id];
    p.uv = 0;
    p.world = p.pos.xyz;
    return p;
}

// 直接光 Cook-Torrance：GGX 分布、Schlick-GGX 遮蔽及 Schlick 菲涅耳。
// 与 Vulkan GLSL 使用相同场景和参数，方便逐项对照；此处未实现 IBL。
float3 PBR(float3 N, float3 V, float3 P, float metal, float r) {
    const float PI = 3.14159265359;
    float3 albedo = float3(.8, .25, .07), sum = .02 * albedo * (1 - metal),
           f0 = lerp(.04.xxx, albedo, metal);
```
