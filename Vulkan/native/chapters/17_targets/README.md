# 第 17 课 · 帧缓冲、RTV/DSV 与尺寸相关资源 · Vulkan 原生

[54 课目录](../../COURSE.md) · [对照精讲](../../../../docs/compare/07_targets_depth_msaa.md) · [上一课](../16_pipeline_interface/README.md) · [下一课](../18_command_storage/README.md)

## 本课目标与先修

学习目标：解释“帧缓冲、RTV/DSV 与尺寸相关资源”解决的问题，找到实际对象和参数，运行对应实验并用数据/画面证明。先完成前一课，并能说明它交付的对象与完成条件；本课在该依赖上增加新契约。

## 为什么需要这一层

目标包括颜色、深度、样本数和尺寸，不仅是一张 color image。窗口变化时要重建依赖尺寸的附件，更新投影与 viewport，并让旧资源在 GPU 不再使用后再释放。

## Vulkan 原生 的对象、参数与执行推导

dynamic rendering 不创建 VkFramebuffer，但每帧仍要选择正确 image view。MSAA 例子使用多样本颜色/深度，resolve 到单样本交换链图像；管线 sample count 与附件一致。

## 对照 DirectX 12：相同问题的另一种分工

RTV/DSV 是 CPU 侧描述符，用 OMSetRenderTargets 录制。多样本颜色使用自己的 RTV，resolve 前切换源/目的状态。back buffer 不能凭空变成普通多样本目标。

## 关键参数与可检查不变量

| 检查项 | Vulkan 表达 | DX12 表达 | 不变量 / 证据 |
| --- | --- | --- | --- |
| 颜色与深度 | 相同 extent/sample count | 相同 size/sample count | 不能只更新颜色附件 |
| 呈现目标 | 交换链图像单样本 | DXGI back buffer 单样本 | MSAA 先画独立目标再 resolve |
| 重建引用 | image view/target 更新 | RTV/DSV/resource 更新 | 无在途引用后才释放旧目标 |

先为每项指出源码中的创建/绑定位置，以及证明它成立的实际日志、字节或图像。数值中的设备限制以查询为准；固定实验尺寸不当作所有设备的上限。

## nvpro_core2 框架对照

RenderTarget::update 负责尺寸相关分配，之后 ImTexture 更新 view。新 view 的引用与旧 GPU 工作必须分别处理。

这部分是原生 Vulkan 各课的框架对照，框架可运行入口仍为原有六课。如果本节需要新接入工作，会按正文说明；不把原生实验当作已完成的框架实现。

## 运行与实际检查

| 路线 | 实际实验 |
| --- | --- |
| Vulkan 原生 | [vk20_msaa_resolve](../../lessons/vk20_msaa_resolve/README.md) |
| DirectX 12 | [dx20_msaa_resolve](../../../../DirectX12/lessons/dx20_msaa_resolve/README.md) |

从合集根目录执行；普通图形程序 Esc 退出，设备/计算/诊断程序输出结果后结束。从“开始学习.cmd”进入本页时，表格可直接点击运行。

```powershell
cmake --preset vulkan-native
cmake --build --preset vulkan-native-debug --target vk20_msaa_resolve --parallel 4
.\Vulkan\native\build\bin\Debug\vk20_msaa_resolve.exe
ctest --preset vulkan-native-debug -R "^vk20_msaa_resolve$" --output-on-failure
```

检查三个层次：创建/绑定/提交的接口约束；数据读回或目标画面；资源使用结束后的回收。诊断实验用合法信息或查询证明渠道可用，不故意保留 API 错误。图形 smoke 的像素检查与数值实验的逐项检查不同，具体实现见源码。

## 关键实现：从当前源码定位

来自 [Vulkan/native/common/native_app.hpp](../../common/native_app.hpp#L412)，第 412—455 行。下列是当前仓库的局部切片，可能在函数中间结束；完整上下文请打开文件，切片不作为单独可编译程序。

```cpp
            VkRenderingAttachmentInfo color{VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
            color.imageView =
                options.samples == VK_SAMPLE_COUNT_1_BIT ? targetView : swap.multisample.view;
            if (options.samples != VK_SAMPLE_COUNT_1_BIT) {
                color.resolveMode = VK_RESOLVE_MODE_AVERAGE_BIT;
                color.resolveImageView = targetView;
                color.resolveImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            }
            color.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            color.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            color.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            color.clearValue.color = {{.035f, .055f, .09f, 1}};
            VkRenderingAttachmentInfo depth{VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
            depth.imageView = swap.depth.view;
            depth.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
            depth.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            depth.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            depth.clearValue.depthStencil = {1, 0};
            VkRenderingInfo render{VK_STRUCTURE_TYPE_RENDERING_INFO};
            render.renderArea = {{0, 0}, swap.extent};
            render.layerCount = 1;
            render.colorAttachmentCount = 1;
            render.pColorAttachments = &color;
            render.pDepthAttachment = &depth;
            vkCmdBeginRendering(cmd, &render);
            if (swap.scene) {
                float time = smoke ? 1.f : float(glfwGetTime());
                if (options.update)
                    options.update(*swap.scene, time);
                swap.scene->render(cmd, swap.extent, time);
            }
            vkCmdEndRendering(cmd);
            if (mode == 10) {
                barrier(cmd,
                        target,
                        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                        VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                        VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                        VK_PIPELINE_STAGE_2_BLIT_BIT,
                        VK_ACCESS_2_TRANSFER_READ_BIT);
                barrier(cmd,
                        swap.images[imageIndex],
                        VK_IMAGE_LAYOUT_UNDEFINED,
```

阅读时按“前置对象 → 参数/布局 → 录制/调用 → 完成/可见性 → 释放”五步标注。共享 helper 减少重复代码，但不会取消每课的 API 契约。

## 受控修改实验

列出 resize 影响的对象，并区分需要重建和只需更新参数的内容。

先记录预测，再只改一个因素；保留基线与修改后的输出。不要同时改变 shader、资源范围和同步，这样即使结果变好也无法归因。

### 参考推导与通过标准

颜色/深度/离屏存储及其 view 常需重建；viewport/scissor/aspect 更新；固定格式未变时管线不一定重建，具体取决于其不可动态状态。

## 常见失败与排查顺序

只调整颜色附件而保留旧尺寸深度，或管线仍为单样本，都会使目标组合不一致。

先看第一条错误和当前执行路径，再看实际数据/绑定，最后看 shader 与画面。需要 GPU 证据时抓取正常交互帧；仅四帧的 smoke 适合自动检查，不适合手动抓帧。

## 自测与本课交付

能否不看源码回答：列出 resize 影响的对象，并区分需要重建和只需更新参数的内容。

提交一份简短记录：输入与假设、唯一修改、预期、实际证据、原因、是否恢复基线。如果只能复述函数名而说不清何时允许访问/释放，先回到本课的数据流。

## 官方阅读位置与当前工程差异

[指定 Vulkan 教程对应位置](https://docs.vulkan.net.cn/tutorial/latest/03_Drawing_a_triangle/03_Drawing/00_Framebuffers.html)。DX12 路线按同一概念顺序建立映射，其对象与约束以 Microsoft 文档和本后端代码为准。49—54 课包含在基础/compute 之后加入的工程扩展。

[版本与资料说明](../../../../docs/REFERENCES.md) · [调试](../../../../docs/DEBUGGING.md) · [功能完成度](../../../../docs/FEATURE_MATRIX.md)

## 固定输入的实际 GPU 读回

![vk20_msaa_resolve](../../../../docs/previews/vk20_msaa_resolve.png)
