# 07 · 呈现目标、深度、离屏与 MSAA

[对照路线](../COMPARE.md) · [交互式对照](index.html#07_targets_depth_msaa) · [上一章](06_textures_color.md) · [下一章](08_synchronization.md)

渲染目标不仅是一张图片，还包含格式、尺寸、样本数、访问方式和清除/保留策略。本章区分“画到哪里”和“什么时候可以显示”。

## 默认目标与应用自己的目标

OpenGL 的默认 framebuffer 由窗口系统关联，FBO 则把应用纹理/renderbuffer 组织成附件。Vulkan 的交换链图像由实现提供，应用为其创建 image view；深度和离屏图片由应用分配。DXGI back buffer 同样由交换链提供，RTV/DSV 描述写入方式，自己的离屏目标是独立 resource。

| 目标属性 | OpenGL | Vulkan | DX12 |
| --- | --- | --- | --- |
| 颜色访问 | FBO attachment | image view + rendering attachment | RTV |
| 深度访问 | depth attachment | D32 image/view | D32 resource + DSV |
| 清除 | glClear 及 mask | loadOp=CLEAR / clearValue | ClearRenderTargetView / ClearDepthStencilView |
| 保存 | 附件继续存在 | storeOp 决定是否保留 | 资源内容与后续命令决定 |
| 显示 | SwapBuffers | QueuePresentKHR | swap chain Present |

Vulkan dynamic rendering 在开始时直接指定附件，管线仍声明相容格式。若创建 pipeline 时颜色格式与实际目标不同，省略 render pass 也不会让它合法。nvpro 的 RenderTarget 帮忙分配附件和填 rendering state；你仍要知道它的格式与图像用途。

## 深度测试的完整条件

正确遮挡需要深度存储、深度测试启用、合适比较函数、写入状态和每帧清除。当前显式后端用 D32、清为 1、LESS，匹配普通 0..1 深度投影。把 clear 改为 0 却继续 LESS，绝大多数可见片段会失败。OpenGL 还要注意 depth mask；透明遍关闭深度写入后必须在后续不透明遍恢复。

本代码没有使用 reversed-Z。以后迁移时要一起改变投影、比较函数与 clear value，不能只把 LESS 换成 GREATER。还要检查阴影和深度重构公式是否采用同一约定。

## 离屏与后处理不是同一个动作

OpenGL 11 把场景纹理交给全屏 shader，可以改变颜色。vk10 是先离屏画场景，再 blit 到交换链；dx10 是 CopyResource。后两者已经演示额外目标和传输依赖，但没有完整的后处理材质、Bloom 或 tone-mapping pass。CopyResource 也不像可着色的全屏 draw 那样能任意改变图像内容。

如果要把 Bloom 移植过去，至少需要浮点 scene image、亮部提取、ping-pong blur targets、最终 composite pipeline 和各遍的依赖。先让离屏结果保持原样输出，再逐项加入效果，容易定位中间环节。

## MSAA 的当前范围与迁移条件

OpenGL 29 已有多样本颜色/深度 renderbuffer 和单样本 resolve 目标。基础课程保留单样本，新增两后端 20 已完成 4x MSAA 和 resolve。新增实现先检查设备支持的 sample count，创建相同样本数的颜色/深度目标，并让 pipeline/PSO 匹配；最终 resolve 到可呈现的单样本图像。MSAA 主要处理几何覆盖边缘，不等于消除所有纹理闪烁或 shader 内的高频变化。

## 窗口变化实验

重复缩放、最小化再恢复窗口。预期没有除零、旧尺寸访问或使用已销毁目标。Vulkan 遇到 OUT_OF_DATE 要重建；DX12 ResizeBuffers 前释放旧 back buffer 引用并等 GPU；OpenGL 自建 FBO 的附件存储也需要随尺寸变化。相机 aspect 和 viewport 是另外两项检查，不由新建纹理自动更新。

自测：为什么换了 FBO 画面只占屏幕角落？常见答案是 viewport 还保留离屏小尺寸。目标绑定与 viewport 是独立状态。

<!-- generated-source-comparison -->

## 与本仓库实现并排核对

以下切片直接取自当前仓库，省略了前后文。行号用于定位，不能把片段当作完整可编译程序。

| 路线 | 可运行参照 / 范围 |
| --- | --- |
| OpenGL | [29_msaa](../../OpenGL/lessons/29_msaa/README.md) |
| Vulkan 原生 | [vk20_msaa_resolve](../../Vulkan/native/lessons/vk20_msaa_resolve/README.md) |
| Vulkan / nvpro_core2 | [nv02_application](../../Vulkan/nvpro/lessons/nv02_application/README.md) |
| DirectX 12 | [dx20_msaa_resolve](../../DirectX12/lessons/dx20_msaa_resolve/README.md) |

### OpenGL · 已实现

多样本绘制与 resolve 是两个步骤；MSAA 不消除所有纹理闪烁。

[OpenGL/lessons/29_msaa/main.cpp](../../OpenGL/lessons/29_msaa/main.cpp#L38)，第 38—61 行：

```cpp
        glBindFramebuffer(GL_READ_FRAMEBUFFER, msaa.fbo);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, resolved.fbo);
        glBlitFramebuffer(0,
                          0,
                          app.width,
                          app.height,
                          0,
                          0,
                          app.width,
                          app.height,
                          GL_COLOR_BUFFER_BIT,
                          GL_NEAREST);
        defaultTarget(app);
        glDisable(GL_DEPTH_TEST);
        screen.use();
        screen.set("tex", 0);
        screen.set("grayscale", 0);
        bind2D(resolved.colors[0], 0);
        plane.draw();
        app.end();
    }
    return 0;
} catch (const std::exception& e) {
    return failure(e);
```

### Vulkan 原生 · 已实现

4x 颜色/深度与 dynamic rendering resolve，目标与管线样本数一致。

[Vulkan/native/lessons/vk20_msaa_resolve/support/native_app.hpp](../../Vulkan/native/lessons/vk20_msaa_resolve/support/native_app.hpp#L417)，第 417—452 行：

```cpp
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
```

### Vulkan / nvpro_core2 · 已实现

尺寸变化重建目标后，也要更新用于 UI 的 image view。

[Vulkan/nvpro/lessons/nv02_application/support/nvapp_course.hpp](../../Vulkan/nvpro/lessons/nv02_application/support/nvapp_course.hpp#L108)，第 108—115 行：

```cpp
    void onResize(VkCommandBuffer cmd, const VkExtent2D& size) override {
        vkCheck(target.update(cmd, size));
        if (!smoke)
            display.update(target.getUiImageView());
    }
    void onUIRender() override {
        if (ImGui::IsKeyPressed(ImGuiKey_Escape))
            app->close();
```

### DirectX 12 · 已实现

4x 目标经 ResolveSubresource 到单样本 back buffer。

[DirectX12/lessons/dx20_msaa_resolve/support/dx12_course.hpp](../../DirectX12/lessons/dx20_msaa_resolve/support/dx12_course.hpp#L653)，第 653—688 行：

```cpp
            c.cmd->ResolveSubresource(
                back.Get(), 0, multisample.Get(), 0, DXGI_FORMAT_R8G8B8A8_UNORM);
            auto restore = transition(multisample.Get(),
                                      D3D12_RESOURCE_STATE_RESOLVE_SOURCE,
                                      D3D12_RESOURCE_STATE_RENDER_TARGET);
            c.cmd->ResourceBarrier(1, &restore);
            old = D3D12_RESOURCE_STATE_RESOLVE_DEST;
        }
        ComPtr<ID3D12Resource> readback;
        D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint{};
        if (capturePath) {
            auto b = transition(back.Get(), old, D3D12_RESOURCE_STATE_COPY_SOURCE);
            c.cmd->ResourceBarrier(1, &b);
            auto desc = back->GetDesc();
            UINT rows;
            UINT64 bytes, total;
            c.device->GetCopyableFootprints(&desc, 0, 1, 0, &footprint, &rows, &bytes, &total);
            readback = c.buffer(total, D3D12_HEAP_TYPE_READBACK, D3D12_RESOURCE_STATE_COPY_DEST);
            D3D12_TEXTURE_COPY_LOCATION src{}, dst{};
            src.pResource = back.Get();
            src.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
            dst.pResource = readback.Get();
            dst.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
            dst.PlacedFootprint = footprint;
            c.cmd->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);
            old = D3D12_RESOURCE_STATE_COPY_SOURCE;
        }

        // 呈现前把 back buffer 恢复到 PRESENT；CPU 读回则在 GPU 拷贝及 fence 完成后进行。
        auto present = transition(back.Get(), old, D3D12_RESOURCE_STATE_PRESENT);
        c.cmd->ResourceBarrier(1, &present);
        c.submit();
        dxCheck(swap->Present(1, 0));
        c.wait();
        if (capturePath) {
            void* p;
```
