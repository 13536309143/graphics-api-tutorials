# nv03_triangle · 动态渲染与共享场景

[路线首页](../../README.md) · [原生 Vulkan 对照](../../../native/README.md)

## 构建与运行

```powershell
cmake --preset vulkan-nvpro
cmake --build --preset vulkan-nvpro-debug --target nv03_triangle --parallel 4
.\Vulkan\nvpro\build\bin\Debug\nv03_triangle.exe
```

以上命令从仓库根目录执行，也可使用本页“运行本课”。预期：Viewport 显示彩色三角形。

## 代码阅读

先读 [main.cpp](main.cpp)，再读路线根目录 common/nvapp_course.hpp 的 NvLesson/nvproMain。框架实现位于 external/nvpro_core2；其固定 commit 和许可证见路线 README。共享底层实现位于 Vulkan/common/scene_vk.hpp，公共数学/几何位于 shared/scene.hpp。

nvvk::RenderTargetState 把附件状态整理为动态渲染信息，cmdBeginRendering 开始 pass；底层图形 pipeline 与 shader 仍通过 Vulkan 代码构建。此处刻意与 native 共用 SceneVk，方便把“框架负责窗口/目标”和“应用仍负责 shader/绘制状态”区分开。GENERAL 布局不代表可以省略图像写后读屏障。

## 动手练习

读 RenderTargetState::cmdBeginRendering，对照原生 VkRenderingInfo；把 Viewport 内容保存并核对读回。

自测：列出本课中由 nvpro_core2 管理的对象，以及仍由 SceneVk/应用管理的对象。参考：Context/Application/RenderTarget/Allocator 负责各自对象，shader、pipeline layout、descriptor 与绘制逻辑仍有显式 Vulkan 状态；框架不会自动推断任意自定义 pass 的全部依赖。

## 检查

`nv03_triangle.exe --smoke` 使用 GPU 无窗口运行或控制台检查。渲染课读回 Viewport 图像为 PPM；计算课逐项验证。验证层错误会使测试失败。常规窗口的 resize/minimize、UI 输入和长时间运行需要另外检查，不能以 headless 通过代替全部交互验证。


## 实测画面

固定时间的 GPU 读回图像（nvpro 为 headless Viewport 输出）。

![nv03_triangle](../../../../docs/previews/nv03_triangle.png)


## 深入学习：数据、接口与执行

onRender 用 RenderTargetState 开始动态渲染，SceneVk 录制三角形，再把颜色写依赖连接到 UI 采样。GENERAL 布局保留也不消除这条写后读关系。

配套精讲：[三 API 对照 · 三角形、管线与着色器](../../../../docs/compare/02_triangle_pipeline.md)。先读概念与参数选择，再返回本课实现核对，不必一次展开所有公共类。

### 本课的真实源码切片

来自 [Vulkan/nvpro/common/nvapp_course.hpp](../../common/nvapp_course.hpp#L130)，第 130—159 行；这是现有实现的局部摘录，末尾可能处于函数中间，不是可单独编译的完整程序。

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
```

### 同主题跨 API 导航

以下是同主题参考；场景、功能完成度和版本基线可能不同，具体边界见对照章节。

| 路线 | 可运行参照 / 当前范围 |
| --- | --- |
| OpenGL | [02_triangle](../../../../OpenGL/lessons/02_triangle/README.md) |
| Vulkan 原生 | [vk03_triangle](../../../native/lessons/vk03_triangle/README.md) |
| Vulkan / nvpro_core2 | [nv03_triangle](README.md) |
| DirectX 12 | [dx03_triangle](../../../../DirectX12/lessons/dx03_triangle/README.md) |

### 先预测，再验证

省略最后屏障，某驱动仍显示正常能证明安全吗？

参考推导：不能。偶然顺序不等于合法依赖；应按消费者阶段和访问推导，并用同步验证辅助检查。

在一次实验中只改一个因素，记录预测、实际画面/日志和原因。检查已有 smoke 是否仍通过；图形调试器里再核对本次 draw 的实际输入/输出，不能仅凭进程正常退出下结论。
