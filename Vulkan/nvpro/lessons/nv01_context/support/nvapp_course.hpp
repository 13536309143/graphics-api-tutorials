// SPDX-License-Identifier: Apache-2.0
// Application-element lifecycle adapted from NVIDIA nvpro_core2 project_template
// (Copyright NVIDIA CORPORATION); see root NOTICE and the pinned submodule LICENSE.
#pragma once
#define VMA_IMPLEMENTATION
#include <volk.h>
#include <nvapp/application.hpp>
#include <nvapp/imgui_texture.hpp>
#include <nvvk/context.hpp>
#include <nvvk/resource_allocator.hpp>
#include <nvvk/render_target.hpp>
#include <nvvk/staging.hpp>
#include <GLFW/glfw3.h>
#include "scene_vk.hpp"
#include <memory>
#include <atomic>
namespace course {
inline std::filesystem::path nvShaders;
inline std::atomic<int> nvValidationErrors = 0;
inline VKAPI_ATTR VkBool32 VKAPI_CALL nvValidation(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
                                                   VkDebugUtilsMessageTypeFlagsEXT type,
                                                   const VkDebugUtilsMessengerCallbackDataEXT* data,
                                                   void*) {
    if ((severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) &&
        (type & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT)) {
        ++nvValidationErrors;
        std::cerr << "VALIDATION ERROR: " << data->pMessage << '\n';
    }
    return VK_FALSE;
}
struct NvContextOwner {
    nvvk::Context context;
    VkDebugUtilsMessengerEXT messenger{};
    ~NvContextOwner() {
        if (context.getDevice())
            vkDeviceWaitIdle(context.getDevice());
        if (messenger)
            vkDestroyDebugUtilsMessengerEXT(context.getInstance(), messenger, nullptr);
        context.deinit();
    }
};

// 框架通过 onAttach/onResize/onRender/onDetach 驱动生命周期；SceneVk 仍负责底层绘制。
class NvLesson : public nvapp::IAppElement {
    nvapp::Application* app{};
    VkContextRef c;
    nvvk::ResourceAllocator allocator;
    nvvk::RenderTarget target;
    nvapp::ImTexture display;
    std::unique_ptr<SceneVk> scene;
    nvvk::Buffer uploadedVertices;
    int mode;
    bool smoke;
    float roughnessScale = 1;
    std::filesystem::path output;

public:
    NvLesson(int m, bool test, const std::filesystem::path& path)
        : mode(m), smoke(test), output(path) {
    }
    void onAttach(nvapp::Application* application) override {
        app = application;
        auto queue = app->getQueue(0);
        c = {app->getInstance(),
             app->getPhysicalDevice(),
             app->getDevice(),
             queue.queue,
             queue.familyIndex};

        // VMA 管理分配与绑定，调用方仍需声明用途并保证 GPU 使用期间资源存活。
        VmaAllocatorCreateInfo ai{};
        ai.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
        ai.physicalDevice = c.physical;
        ai.device = c.device;
        ai.instance = c.instance;
        ai.vulkanApiVersion = VK_API_VERSION_1_4;
        vkCheck(allocator.init(ai));
        nvvk::RenderTarget::CreateInfo info{};
        info.alloc = &allocator;
        info.colorFormats = {VK_FORMAT_R8G8B8A8_UNORM};
        info.depthFormat = VK_FORMAT_D32_SFLOAT;
        info.colorUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT |
                          VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        info.debugName = "Course viewport";
        vkCheck(target.init(info));
        if (mode >= 3)
            scene = std::make_unique<SceneVk>(c, mode, VK_FORMAT_R8G8B8A8_UNORM, nvShaders);
        if (mode == 6) {
            auto& vertices = scene->mesh.vertices;
            vkCheck(allocator.createBuffer(uploadedVertices,
                                           vertices.size() * sizeof(Vertex),
                                           VK_BUFFER_USAGE_2_VERTEX_BUFFER_BIT));

            // 先把数据加入 staging，再录制拷贝并等待 GPU；完成后才能释放临时上传器。
            nvvk::StagingUploader upload;
            upload.init(&allocator);
            vkCheck(upload.appendBuffer(uploadedVertices, 0, std::span<const Vertex>(vertices)));
            auto cmd = app->createTempCmdBuffer();
            upload.cmdUploadAppended(cmd);
            app->submitAndWaitTempCmdBuffer(cmd);
            upload.deinit();
            scene->overrideVertices = uploadedVertices.buffer;
            std::cout << "VMA device-local vertex upload complete\n";
        }
    }

    // 视口大小变化时重建 RenderTarget，并把新的 image view 交给 ImGui 显示。
    void onResize(VkCommandBuffer cmd, const VkExtent2D& size) override {
        vkCheck(target.update(cmd, size));
        if (!smoke)
            display.update(target.getUiImageView());
    }
    void onUIRender() override {
        if (ImGui::IsKeyPressed(ImGuiKey_Escape))
            app->close();
        if (smoke)
            return;
        ImGui::Begin("Settings");
        ImGui::TextUnformatted("Vulkan / nvpro_core2 course");
        ImGui::Text("Scene mode: %d", mode);
        if (mode >= 9)
            ImGui::SliderFloat("Roughness scale", &roughnessScale, .2f, 2.f);
        ImGui::TextUnformatted("Esc: close. Resize the viewport to rebuild attachments.");
        ImGui::End();
        ImGui::Begin("Viewport");
        if (target.getSize().width)
            ImGui::Image(display, ImGui::GetContentRegionAvail());
        ImGui::End();
    }
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
            c, VkDeviceSize(size.width) * size.height * 4, VK_BUFFER_USAGE_TRANSFER_DST_BIT);
        immediate(c, [&](VkCommandBuffer cmd) {
            barrier(cmd,
                    target.getColorImage(),
                    VK_IMAGE_LAYOUT_GENERAL,
                    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                    VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
                    VK_ACCESS_2_MEMORY_WRITE_BIT,
                    VK_PIPELINE_STAGE_2_COPY_BIT,
                    VK_ACCESS_2_TRANSFER_READ_BIT);
            VkBufferImageCopy copy{};
            copy.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
            copy.imageExtent = {size.width, size.height, 1};
            vkCmdCopyImageToBuffer(cmd,
                                   target.getColorImage(),
                                   VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                   readback.buffer,
                                   1,
                                   &copy);
            VkMemoryBarrier2 host{VK_STRUCTURE_TYPE_MEMORY_BARRIER_2};
            host.srcStageMask = VK_PIPELINE_STAGE_2_COPY_BIT;
            host.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
            host.dstStageMask = VK_PIPELINE_STAGE_2_HOST_BIT;
            host.dstAccessMask = VK_ACCESS_2_HOST_READ_BIT;
            VkDependencyInfo dependency{VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
            dependency.memoryBarrierCount = 1;
            dependency.pMemoryBarriers = &host;
            // 可见性依赖：源阶段/访问描述生产者，目标阶段/访问描述接下来的消费者。
            vkCmdPipelineBarrier2(cmd, &dependency);
            barrier(cmd,
                    target.getColorImage(),
                    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                    VK_IMAGE_LAYOUT_GENERAL,
                    VK_PIPELINE_STAGE_2_COPY_BIT,
                    VK_ACCESS_2_TRANSFER_READ_BIT,
                    VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
                    VK_ACCESS_2_MEMORY_READ_BIT);
        });
        void* p;
        vkCheck(vkMapMemory(c.device, readback.memory, 0, readback.size, 0, &p));
        capture(
            output, static_cast<uint8_t*>(p), int(size.width), int(size.height), false, mode >= 3);
        vkUnmapMemory(c.device, readback.memory);
    }

    // 先等待 GPU，再依次释放依赖 allocator 的场景、目标和缓冲，最后关闭 allocator。
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

} // namespace course
