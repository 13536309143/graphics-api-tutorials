// 遮挡、清除、比较与写入
// 本目录拥有完整课程源码；只共享第三方依赖，不链接其他章节的课程实现。
// 本章检查：普通投影；当前 clear=1 / LESS。
// 本章检查：写入与测试；保留测试不等于会写深度。
// 本章检查：历史清理；不依赖上一帧残留。
// 读代码顺序：资源所有权 → 管线/绑定 → 录制和提交 → 完成检查 → 逆序释放。
#include "support/native_app.hpp"
namespace course {
int runChapter(int argc, char** argv) {
    NativeRenderOptions options;

    bool smoke = false;
    for (int i = 1; i < argc; ++i)
        smoke |= std::strcmp(argv[i], "--smoke") == 0;
    validationErrors = 0;
    nativeShaders =
        std::filesystem::absolute(argv[0]).parent_path() / COURSE_RUNTIME_NAME / "shaders";
    // 设备探测和计算课不创建窗口，也不需要 surface 或交换链。
    if (!glfwInit())
        throw std::runtime_error("GLFW initialization failed");
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    if (smoke)
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    auto window = glfwCreateWindow(960, 640, "Native Vulkan Course", nullptr, nullptr);
    if (!window)
        throw std::runtime_error("Window creation failed");
    {
        NativeDevice d(window);
        Swapchain swap(d, window, 7, options);
        VkCommandPoolCreateInfo pi{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
        pi.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        pi.queueFamilyIndex = d.c.family;
        VkCommandPool pool;
        vkCheck(vkCreateCommandPool(d.c.device, &pi, nullptr, &pool));
        VkCommandBufferAllocateInfo ai{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
        ai.commandPool = pool;
        ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        ai.commandBufferCount = 1;
        VkCommandBuffer cmd;
        vkCheck(vkAllocateCommandBuffers(d.c.device, &ai, &cmd));
        VkSemaphoreCreateInfo si{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
        VkSemaphore acquired;
        vkCheck(vkCreateSemaphore(d.c.device, &si, nullptr, &acquired));
        VkFenceCreateInfo fi{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
        fi.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        VkFence fence;
        vkCheck(vkCreateFence(d.c.device, &fi, nullptr, &fence));
        int frames = 0;
        while (!glfwWindowShouldClose(window) && (!smoke || frames < 4)) {
            glfwPollEvents();
            if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
                break;
            int w, h;
            glfwGetFramebufferSize(window, &w, &h);
            if (w < 1 || h < 1) {
                glfwWaitEvents();
                continue;
            }
            if (uint32_t(w) != swap.extent.width || uint32_t(h) != swap.extent.height)
                swap.recreate();
            // 先等待上一轮命令完成，再复用命令缓冲和共享 UBO；本课程采用单帧串行同步。
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
            VkImage target = (swap.images[imageIndex]);
            VkImageView targetView = (swap.views[imageIndex]);
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
                    VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
                    VK_PIPELINE_STAGE_2_NONE,
                    0,
                    VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT |
                        VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
                    VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                    VK_IMAGE_ASPECT_DEPTH_BIT);
            if (options.samples != VK_SAMPLE_COUNT_1_BIT)
                barrier(cmd,
                        swap.multisample.image,
                        VK_IMAGE_LAYOUT_UNDEFINED,
                        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                        VK_PIPELINE_STAGE_2_NONE,
                        0,
                        VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                        VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT);
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
            bool captureFrame = smoke && frames == 3;
            std::unique_ptr<VkBufferResource> readback;
            if (captureFrame) {
                readback = std::make_unique<VkBufferResource>();
                readback->init(d.c,
                               VkDeviceSize(swap.extent.width) * swap.extent.height * 4,
                               VK_BUFFER_USAGE_TRANSFER_DST_BIT);
                barrier(cmd,
                        swap.images[imageIndex],
                        (VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL),
                        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                        (VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT),
                        (VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT),
                        VK_PIPELINE_STAGE_2_COPY_BIT,
                        VK_ACCESS_2_TRANSFER_READ_BIT);
                VkBufferImageCopy copy{};
                copy.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
                copy.imageExtent = {swap.extent.width, swap.extent.height, 1};
                vkCmdCopyImageToBuffer(cmd,
                                       swap.images[imageIndex],
                                       VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                       readback->buffer,
                                       1,
                                       &copy);
                VkMemoryBarrier2 host{VK_STRUCTURE_TYPE_MEMORY_BARRIER_2};
                host.srcStageMask = VK_PIPELINE_STAGE_2_COPY_BIT;
                host.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
                host.dstStageMask = VK_PIPELINE_STAGE_2_HOST_BIT;
                host.dstAccessMask = VK_ACCESS_2_HOST_READ_BIT;
                VkDependencyInfo dep{VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
                dep.memoryBarrierCount = 1;
                dep.pMemoryBarriers = &host;
                // 可见性依赖：源阶段/访问描述生产者，目标阶段/访问描述接下来的消费者。
                vkCmdPipelineBarrier2(cmd, &dep);
            }
            barrier(cmd,
                    swap.images[imageIndex],
                    captureFrame ? VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
                                 : (VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL),
                    VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                    captureFrame ? VK_PIPELINE_STAGE_2_COPY_BIT
                                 : (VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT),
                    captureFrame ? VK_ACCESS_2_TRANSFER_READ_BIT
                                 : (VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT),
                    VK_PIPELINE_STAGE_2_NONE,
                    0);
            vkCheck(vkEndCommandBuffer(cmd));
            VkPipelineStageFlags stage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
            VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};
            submit.waitSemaphoreCount = 1;
            submit.pWaitSemaphores = &acquired;
            submit.pWaitDstStageMask = &stage;
            submit.commandBufferCount = 1;
            submit.pCommandBuffers = &cmd;
            submit.signalSemaphoreCount = 1;
            // 呈现等待的信号量按交换链图像分配，在该图像再次 acquire 后才能安全复用。
            VkSemaphore finished = swap.presentReady[imageIndex];
            submit.pSignalSemaphores = &finished;
            // 提交只把工作交给 GPU；复用命令、常量或释放资源前仍需确认完成。
            vkCheck(vkQueueSubmit(d.c.queue, 1, &submit, fence));
            // 提交等待 acquire、发出渲染完成信号；present 等该信号后才可读取交换链图像。
            VkPresentInfoKHR present{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
            present.waitSemaphoreCount = 1;
            present.pWaitSemaphores = &finished;
            present.swapchainCount = 1;
            present.pSwapchains = &swap.handle;
            present.pImageIndices = &imageIndex;
            auto result = vkQueuePresentKHR(d.c.queue, &present);
            // 为便于学习逐帧等待；多帧并行需为 UBO、命令缓冲和临时资源分别管理生命周期。
            vkCheck(vkQueueWaitIdle(d.c.queue));
            if (readback) {
                void* p;
                vkCheck(vkMapMemory(d.c.device, readback->memory, 0, readback->size, 0, &p));
                capture(std::filesystem::path(argv[0]).parent_path() /
                            (options.captureStem.empty() ? "native_" + std::to_string(7) + ".ppm"
                                                         : options.captureStem + ".ppm"),
                        static_cast<uint8_t*>(p),
                        int(swap.extent.width),
                        int(swap.extent.height),
                        swap.format == VK_FORMAT_B8G8R8A8_UNORM ||
                            swap.format == VK_FORMAT_B8G8R8A8_SRGB,
                        7 >= 3);
                vkUnmapMemory(d.c.device, readback->memory);
            }
            if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
                swap.recreate();
            else
                vkCheck(result);
            ++frames;
        }
        vkDeviceWaitIdle(d.c.device);
        vkDestroyFence(d.c.device, fence, nullptr);
        vkDestroySemaphore(d.c.device, acquired, nullptr);
        vkDestroyCommandPool(d.c.device, pool, nullptr);
    }
    glfwDestroyWindow(window);
    glfwTerminate();
    std::cout << "Validation errors: " << validationErrors << '\n';
    return validationErrors ? 1 : 0;
}
} // namespace course

int main(int argc, char** argv) try {
    return course::runChapter(argc, argv);
} catch (const std::exception& error) {
    std::cerr << "ERROR: " << error.what() << '\n';
    return 1;
}
