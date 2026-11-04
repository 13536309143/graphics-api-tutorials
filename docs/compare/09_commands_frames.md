# 09 · 命令录制、帧资源与两帧并行设计

[对照路线](../COMPARE.md) · [交互式对照](index.html#09_commands_frames) · [上一章](08_synchronization.md) · [下一章](10_compute_readback.md)

本仓库显式后端的单帧串行版本容易检查：等完成、更新一份数据、录制、提交、再等。学到这里之后，再设计 CPU/GPU 重叠才不会把偶然正确误认为可扩展。

## 命令与执行分开

Vulkan `vkCmd*` 写入 command buffer，DX12 `cmd->*` 写入 command list。它们通常不在调用时立即执行 GPU 操作。OpenGL 则由驱动把 context 调用转换成后续 GPU 工作；函数调用看似直接，也不等于 GPU 与 CPU 同步运行。

Vulkan command pool 决定分配/重置与队列族等要求；DX12 command allocator 保存命令所用存储，command list 负责录制接口。某个 command list 对象能够再次 Reset，与它背后 allocator 的存储可以被覆盖不是同一个条件。当前 `DxDevice::submit` 等待 fence，所以下一次 begin 可以安全 Reset allocator。

## 帧槽不等于交换链图像

假设有两份 CPU 帧资源、三张交换链图像。frameSlot 按 0、1 轮换，用于 UBO、allocator/pool 等；imageIndex 来自 acquire/GetCurrentBackBufferIndex，用于当前颜色图像。它们可能恰好一样，也可能完全不同。用 frameSlot 索引三张图像或用 imageIndex 索引两份 UBO，都可能越界或复用错误。

| 资源 | 两帧设计中的处理 | 原因 |
| --- | --- | --- |
| 可变帧常量 | 每帧槽独立范围 | CPU 不覆盖 GPU 正在读的数据 |
| command allocator/pool | 每帧槽管理并等待对应完成值 | 不覆盖在途命令存储 |
| 静态顶点/纹理 | 通常可共享只读 | 内容不变且上传已完成 |
| 深度/离屏目标 | 分帧分配或明确串行依赖 | 多帧可能同时写同一目标 |
| present 等待信号 | 按呈现资源规则管理 | 提交结束与呈现消费完成不是同一事件 |

## 设计草图，不是现成后端

```text
slot = frameNumber % framesInFlight
等待 slot 上次提交的完成条件
选择本次交换链 imageIndex
只更新 slot 对应的常量与命令存储
录制图像/缓冲访问和渲染命令
提交并记录 slot 的完成标记
按呈现同步规则交回 imageIndex
```

这是算法顺序，不是可以粘贴替换 nativeMain 的完整代码。还缺错误分支、图像在途跟踪、窗口重建、descriptor 更新规则、临时 upload 回收，以及 nvpro 框架已有帧调度的协调。先写不变量，再实现：同一帧槽未完成前不写；旧附件无人使用后才销毁；每个 signal 都有可证明的消费/复用规则。

## 比较 nvpro 的边界

nvapp 已有自己的帧循环和命令管理。课程自定义 SceneVk 却共享一份 UBO 和离屏目标，并在 onRender 前等待队列。优化该课时要同时检查 Application 的保证与自定义资源的保证；不能只看到框架有多帧支持，就认为用户资源也天然多帧安全。

## 实验与验收

先不修改行为，在现有版本记录 CPU frame number、frameSlot（设计值）、imageIndex、fence 完成值和资源地址。然后在独立练习分支引入两份 Frame，只运行固定场景，确认同一槽的下一次写发生在其完成之后。最后才移除相应串行等待。

验收同时包含验证层零错误、重复 resize、固定输入画面稳定，以及计时中确实出现预期重叠。帧率变高但矩阵偶尔跳动不是成功。不能用 sleep 延迟来证明同步正确，CPU 调度变化会掩盖问题。

[Microsoft 命令列表说明](https://learn.microsoft.com/en-us/windows/win32/direct3d12/recording-command-lists-and-bundles) 可核对录制对象与执行的区别。新增 vk24/dx24 已实际验证两槽 GPU 计算提交与逐代读回；窗口呈现的完整资源环仍属于迁移设计，基础图形路径保留串行契约。

<!-- generated-source-comparison -->

## 与本仓库实现并排核对

以下切片直接取自当前仓库，省略了前后文。行号用于定位，不能把片段当作完整可编译程序。

| 路线 | 可运行参照 / 范围 |
| --- | --- |
| OpenGL | [07_cube](../../OpenGL/lessons/07_cube/README.md) |
| Vulkan 原生 | [vk02_swapchain](../../Vulkan/native/lessons/vk02_swapchain/README.md) |
| Vulkan / nvpro_core2 | [nv03_triangle](../../Vulkan/nvpro/lessons/nv03_triangle/README.md) |
| DirectX 12 | [dx02_swapchain](../../DirectX12/lessons/dx02_swapchain/README.md) |

### OpenGL · 已实现

SwapBuffers 交出图像，不代表可以用 CPU 调用耗时表示 GPU 耗时。

[OpenGL/common/app.hpp](../../OpenGL/common/app.hpp#L91)，第 91—114 行：

```cpp
    void end() {

        // 自动验证使用固定帧数并在交换缓冲前读回当前后缓冲，检查是否真正绘出几何。
        if (smoke && frames == 3) {
            std::vector<unsigned char> rgb(size_t(width) * height * 3);
            glPixelStorei(GL_PACK_ALIGNMENT, 1);
            glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, rgb.data());
            glPixelStorei(GL_PACK_ALIGNMENT, 4);
            size_t changed = 0;
            for (size_t i = 0; i < rgb.size(); i += 3)
                if (std::abs(int(rgb[i]) - 9) > 3 || std::abs(int(rgb[i + 1]) - 14) > 3 ||
                    std::abs(int(rgb[i + 2]) - 23) > 3)
                    ++changed;
            std::cout << "Rendered non-background pixels: " << changed << '\n';
            int lo = 255, hi = 0;
            for (size_t k = 0; k < rgb.size(); k += 3) {
                int l = int(rgb[k]) + rgb[k + 1] + rgb[k + 2];
                lo = std::min(lo, l / 3);
                hi = std::max(hi, l / 3);
            }
            if (expectGeometry && (changed < 100 || hi - lo < 10))
                throw std::runtime_error("Smoke test: geometry is missing");
            if (capture) {
                std::ofstream out(assets.parent_path() / (std::string(glfwGetWindowTitle(window)) +
```

### Vulkan 原生 · 已实现

成功 acquire 后才 reset fence，避免无提交却等待已重置 fence。

[Vulkan/native/common/native_app.hpp](../../Vulkan/native/common/native_app.hpp#L365)，第 365—395 行：

```cpp
            vkCheck(vkWaitForFences(d.c.device, 1, &fence, VK_TRUE, UINT64_MAX));
            uint32_t imageIndex;

            // acquire semaphore 告知 GPU 图像可用；遇到 OUT_OF_DATE 时先重建再尝试。
            VkResult a = vkAcquireNextImageKHR(
                d.c.device, swap.handle, UINT64_MAX, acquired, {}, &imageIndex);
            if (a == VK_ERROR_OUT_OF_DATE_KHR) {
                swap.recreate();
                continue;
            }
            if (a != VK_SUBOPTIMAL_KHR)
                vkCheck(a);

            // 只有成功获取图像、确定会提交时才重置 fence，避免下次永久等待。
            vkCheck(vkResetFences(d.c.device, 1, &fence));
            vkCheck(vkResetCommandBuffer(cmd, 0));
            VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
            vkCheck(vkBeginCommandBuffer(cmd, &begin));
            VkImage target = mode == 10 ? swap.offscreen.image : swap.images[imageIndex];
            VkImageView targetView = mode == 10 ? swap.offscreen.view : swap.views[imageIndex];
            barrier(cmd,
                    target,
                    VK_IMAGE_LAYOUT_UNDEFINED,
                    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                    VK_PIPELINE_STAGE_2_NONE,
                    0,
                    VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                    VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT);
            barrier(cmd,
                    swap.depth.image,
                    VK_IMAGE_LAYOUT_UNDEFINED,
```

### Vulkan / nvpro_core2 · 已实现

自定义资源和框架帧调度都要满足各自生命周期。

[Vulkan/nvpro/common/nvapp_course.hpp](../../Vulkan/nvpro/common/nvapp_course.hpp#L223)，第 223—234 行：

```cpp
    void onDetach() override {
        vkDeviceWaitIdle(c.device);
        if (!smoke)
            display.deinit();
        scene.reset();
        if (uploadedVertices.buffer)
            allocator.destroyBuffer(uploadedVertices);
        target.deinit();
        allocator.deinit();
    }
};
inline int nvproMain(int argc, char** argv, int chapter) {
```

### DirectX 12 · 已实现

allocator Reset 由前次 submit 的等待保护，多帧版本须改变该契约。

[DirectX12/common/dx12_course.hpp](../../DirectX12/common/dx12_course.hpp#L122)，第 122—147 行：

```cpp
    void begin() {
        dxCheck(allocator->Reset());
        dxCheck(cmd->Reset(allocator.Get(), nullptr));
    }
    void submit() {
        dxCheck(cmd->Close());
        ID3D12CommandList* lists[] = {cmd.Get()};
        queue->ExecuteCommandLists(1, lists);
        wait();
    }

    // 队列 signal fence 后，CPU 等事件确认进度；提交返回不等于 GPU 已完成。
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
