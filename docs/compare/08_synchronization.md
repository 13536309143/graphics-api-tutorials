# 08 · 同步：执行顺序、可见性与生命周期

[对照路线](../COMPARE.md) · [交互式对照](index.html#08_synchronization) · [上一章](07_targets_depth_msaa.md) · [下一章](09_commands_frames.md)

这是显式 API 学习中最值得慢下来的一章。先写“生产者写什么、消费者读什么、在哪个队列、谁需要等待”，再选择 barrier、semaphore 或 fence；不要先找一个能让验证层安静的万能函数。

## 三个独立的问题

执行依赖决定后续操作什么时候可以开始。内存依赖让生产者的写入对消费者可见。生命周期保证资源及其存储在最后一次使用结束前不被释放或覆盖。它们相互关联，但不是一个布尔值。

CPU 执行完录制命令，不表示 GPU 执行完命令。map 成功不表示 GPU 不在读取该内存。把 image layout 写成 SHADER_READ_ONLY，也不表示刚才的传输写入已建立了完整依赖。即使某块内存 coherent，也仍然不能随意与 GPU 并发覆盖。

| 场景 | OpenGL | Vulkan | DX12 |
| --- | --- | --- | --- |
| 常规 API 上传后绘制 | 驱动按 API 规则协调 | 由资源用法和内存依赖保证 | 状态转换及队列执行顺序 |
| GPU 生产结果供后续 GPU 使用 | 按操作类型，部分需 memory barrier | pipeline barrier / semaphore 等 | transition/UAV barrier、跨队列 fence |
| CPU 等 GPU 完成 | GLsync / 等待接口 | fence，或适当的 timeline 等待 | fence + event |
| GPU 等另一个 GPU 提交 | 共享/同步规则 | semaphore | queue Wait on fence |

OpenGL 的“隐式”不代表没有同步成本，也不代表所有 shader 写入都自动对之后任意访问可见。本课程 OpenGL 基线是 3.3，不能直接把后续版本 compute/image store 的规则假设为当前已实现。

## 推导一次纹理上传

生产者是 copy 操作，访问是 transfer write；消费者是 fragment shader，访问是 sampled read。Vulkan 课程从 TRANSFER_DST_OPTIMAL 过渡到 SHADER_READ_ONLY_OPTIMAL，同时用 COPY/TRANSFER_WRITE 到 FRAGMENT_SHADER/SHADER_SAMPLED_READ 的 stage/access 配对。若换成顶点 shader 采样，消费者阶段也应相应改变。

DX12 当前用 legacy transition barrier 从 COPY_DEST 转到 PIXEL_SHADER_RESOURCE。跨队列复制时还要建立队列间 fence 等待，不能因为状态名称正确就省掉提交之间的联系。legacy barrier 与 newer enhanced barriers 的字段体系不同，本项目没有混用它们。

## Fence 与 semaphore 不可机械互换

本原生 Vulkan 帧流程有两条 GPU 侧信号：acquire 表示交换链图像可用于渲染，render-finished 表示渲染完成可供 present 等待。fence 用来让 CPU 判断上次提交完成，从而复用命令缓冲和 UBO。render-finished 按交换链图像分配；CPU frame slot 与 imageIndex 是不同维度。

特别留意 fence 的 reset 时机。若先 reset，acquire 却返回 OUT_OF_DATE 并直接跳到下一轮，那么没有提交会去 signal 此 fence，下次 CPU 就可能一直等。现有代码在成功取得图像后才 reset；这段顺序本身就是需要理解的算法。

## GENERAL 不是“无需屏障”

nvpro 离屏目标在 GENERAL 布局下渲染并供 ImGui 采样。布局可以不变，颜色写入到片元采样读取的依赖仍然存在，因此代码仍调用 barrier。框架帮助管理 target，不会使自定义 pass 的读写冲突自动消失。

## 练习：为结果读回画一张时间表

按顺序写出 dispatch、shader-write→host-read 依赖、提交、CPU 等待、map、逐项检查。DX12 则是 UAV 计算、转换到 copy source、复制到 readback、等 fence、map。答案的关键是：CPU 读取发生在 GPU 完成之后，且读取来源和可见性都正确；只 map 不等待、只等待但用错资源都不成立。

本课程为了清晰按帧等待，有意牺牲重叠。下一章讲如何设计多帧资源，而不是直接删除 wait。[Khronos 同步指南](https://docs.vulkan.org/guide/latest/synchronization.html) 和 [DX12 资源屏障](https://learn.microsoft.com/en-us/windows/win32/direct3d12/using-resource-barriers-to-synchronize-resource-states-in-direct3d-12) 用于核对规则，具体依赖应按实际访问重新推导。

<!-- generated-source-comparison -->

## 与本仓库实现并排核对

以下切片直接取自当前仓库，省略了前后文。行号用于定位，不能把片段当作完整可编译程序。

| 路线 | 可运行参照 / 范围 |
| --- | --- |
| OpenGL | [11_framebuffer](../../OpenGL/lessons/11_framebuffer/README.md) |
| Vulkan 原生 | [vk06_texture_descriptor](../../Vulkan/native/lessons/vk06_texture_descriptor/README.md) |
| Vulkan / nvpro_core2 | [nv03_triangle](../../Vulkan/nvpro/lessons/nv03_triangle/README.md) |
| DirectX 12 | [dx08_compute_uav](../../DirectX12/lessons/dx08_compute_uav/README.md) |

### OpenGL · 已实现

常规有序 FBO→采样由 GL 规则协调；不能推导成所有 GPU 写后读都自动安全。

[OpenGL/lessons/11_framebuffer/main.cpp](../../OpenGL/lessons/11_framebuffer/main.cpp#L12)，第 12—43 行：

```cpp
    while (app.begin()) {
        if (app.width == 0 || app.height == 0) {
            app.end();
            continue;
        }
        camera.update(app);
        target.resize(app.width, app.height);
        // 第一遍：向离屏颜色纹理和深度附件画立方体。
        glBindFramebuffer(GL_FRAMEBUFFER, target.fbo);
        glEnable(GL_DEPTH_TEST);
        app.clear();
        scene.use();
        texture.bind();
        scene.set("tex", 0);
        auto model = glm::rotate(glm::mat4(1), app.time(), glm::vec3(.3f, 1, 0));
        scene.set("mvp", camera.projection(app) * camera.view() * model);
        box.draw();
        // 第二遍：默认帧缓冲上的全屏矩形。按住空格显示原图，松开为灰度。
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDisable(GL_DEPTH_TEST);
        app.clear();
        screen.use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, target.color);
        screen.set("tex", 0);
        screen.set("grayscale", glfwGetKey(app.window, GLFW_KEY_SPACE) != GLFW_PRESS);
        plane.draw();
        app.end();
    }
    return 0;
} catch (const std::exception& e) {
    return failure(e);
```

### Vulkan 原生 · 已实现

屏障声明前后阶段、访问及布局；并非只修改一个枚举。

[Vulkan/common/scene_vk.hpp](../../Vulkan/common/scene_vk.hpp#L129)，第 129—163 行：

```cpp
inline void barrier(VkCommandBuffer cmd,
                    VkImage image,
                    VkImageLayout before,
                    VkImageLayout after,
                    VkPipelineStageFlags2 srcStage,
                    VkAccessFlags2 srcAccess,
                    VkPipelineStageFlags2 dstStage,
                    VkAccessFlags2 dstAccess,
                    VkImageAspectFlags aspect = VK_IMAGE_ASPECT_COLOR_BIT) {
    VkImageMemoryBarrier2 b{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2};
    b.srcStageMask = srcStage;
    b.srcAccessMask = srcAccess;
    b.dstStageMask = dstStage;
    b.dstAccessMask = dstAccess;
    b.oldLayout = before;
    b.newLayout = after;
    b.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    b.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    b.image = image;
    b.subresourceRange = {aspect, 0, 1, 0, 1};
    VkDependencyInfo dependency{VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
    dependency.imageMemoryBarrierCount = 1;
    dependency.pImageMemoryBarriers = &b;
    vkCmdPipelineBarrier2(cmd, &dependency);
}

// 教学用一次性提交：CPU 等 fence 后再释放命令池及临时上传资源。
inline void immediate(const VkContextRef& c, const std::function<void(VkCommandBuffer)>& record) {
    VkCommandPool pool{};
    VkCommandPoolCreateInfo pi{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    pi.queueFamilyIndex = c.family;
    vkCheck(vkCreateCommandPool(c.device, &pi, nullptr, &pool));
    VkCommandBufferAllocateInfo ai{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    ai.commandPool = pool;
    ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
```

### Vulkan / nvpro_core2 · 已实现

GENERAL→GENERAL 仍声明颜色写入与后续采样依赖。

[Vulkan/nvpro/common/nvapp_course.hpp](../../Vulkan/nvpro/common/nvapp_course.hpp#L130)，第 130—177 行：

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

    // 无窗口检查同样读回真实离屏目标；传输写入经 host 屏障后才能由 CPU 检查。
    void onLastHeadlessFrame() override {
        auto size = target.getSize();
        VkBufferResource readback;
        readback.init(
```

### DirectX 12 · 已实现

CPU 等 fence 只解决完成条件；资源状态还要与实际访问一致。

[DirectX12/common/dx12_course.hpp](../../DirectX12/common/dx12_course.hpp#L132)，第 132—145 行：

```cpp
    void wait() {
        dxCheck(queue->Signal(fence.Get(), ++value));
        if (fence->GetCompletedValue() < value) {
            dxCheck(fence->SetEventOnCompletion(value, event));
            if (WaitForSingleObject(event, 30000) != WAIT_OBJECT_0)
                throw std::runtime_error("GPU fence timeout");
        }
    }
    void checkMessages() {
        if (!messages)
            return;
        UINT64 count = messages->GetNumStoredMessagesAllowedByRetrievalFilter();
        for (UINT64 i = 0; i < count; ++i) {
            SIZE_T bytes = 0;
```
