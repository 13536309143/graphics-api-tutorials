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
                                                   const VkDebugUtilsMessengerCallbackDataEXT* data, void*) {
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
    if (context.getDevice()) vkDeviceWaitIdle(context.getDevice());
    if (messenger) vkDestroyDebugUtilsMessengerEXT(context.getInstance(), messenger, nullptr);
    context.deinit();
  }
};
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
  NvLesson(int m, bool test, const std::filesystem::path& path) : mode(m), smoke(test), output(path) {}
  void onAttach(nvapp::Application* application) override {
    app = application;
    auto queue = app->getQueue(0);
    c = {app->getInstance(), app->getPhysicalDevice(), app->getDevice(), queue.queue, queue.familyIndex};
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
    info.colorUsage =
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    info.debugName = "Course viewport";
    vkCheck(target.init(info));
    if (mode >= 3) scene = std::make_unique<SceneVk>(c, mode, VK_FORMAT_R8G8B8A8_UNORM, nvShaders);
    if (mode == 6) {
      auto& vertices = scene->mesh.vertices;
      vkCheck(allocator.createBuffer(uploadedVertices, vertices.size() * sizeof(Vertex),
                                     VK_BUFFER_USAGE_2_VERTEX_BUFFER_BIT));
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
  void onResize(VkCommandBuffer cmd, const VkExtent2D& size) override {
    vkCheck(target.update(cmd, size));
    if (!smoke) display.update(target.getUiImageView());
  }
  void onUIRender() override {
    if (ImGui::IsKeyPressed(ImGuiKey_Escape)) app->close();
    if (smoke) return;
    ImGui::Begin("Settings");
    ImGui::TextUnformatted("Vulkan / nvpro_core2 course");
    ImGui::Text("Scene mode: %d", mode);
    if (mode >= 9) ImGui::SliderFloat("Roughness scale", &roughnessScale, .2f, 2.f);
    ImGui::TextUnformatted("Esc: close. Resize the viewport to rebuild attachments.");
    ImGui::End();
    ImGui::Begin("Viewport");
    if (target.getSize().width) ImGui::Image(display, ImGui::GetContentRegionAvail());
    ImGui::End();
  }
  void onRender(VkCommandBuffer cmd) override {
    // Intentionally one in-flight scene update, so the shared UBO and offscreen target are safe.
    vkCheck(vkQueueWaitIdle(c.queue));
    barrier(cmd, target.getColorImage(), VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL,
            VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT,
            VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT);
    barrier(cmd, target.getDepthImage(), VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL,
            VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, VK_ACCESS_2_MEMORY_WRITE_BIT,
            VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
            VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_IMAGE_ASPECT_DEPTH_BIT);
    nvvk::RenderTargetState state;
    target.fillState(state);
    state.colorAttachments[0].clearValue.color = {{.035f, .055f, .09f, 1}};
    state.depthAttachment.clearValue.depthStencil = {1, 0};
    nvvk::RenderTargetState::AttachmentOps ops;
    state.cmdBeginRendering(cmd, ops);
    if (scene) scene->render(cmd, target.getSize(), smoke ? 1.f : float(ImGui::GetTime()), roughnessScale);
    vkCmdEndRendering(cmd);
    barrier(cmd, target.getColorImage(), VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL,
            VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
            VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT, VK_ACCESS_2_SHADER_SAMPLED_READ_BIT);
  }
  void onLastHeadlessFrame() override {
    auto size = target.getSize();
    VkBufferResource readback;
    readback.init(c, VkDeviceSize(size.width) * size.height * 4, VK_BUFFER_USAGE_TRANSFER_DST_BIT);
    immediate(c, [&](VkCommandBuffer cmd) {
      barrier(cmd, target.getColorImage(), VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
              VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, VK_ACCESS_2_MEMORY_WRITE_BIT, VK_PIPELINE_STAGE_2_COPY_BIT,
              VK_ACCESS_2_TRANSFER_READ_BIT);
      VkBufferImageCopy copy{};
      copy.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
      copy.imageExtent = {size.width, size.height, 1};
      vkCmdCopyImageToBuffer(cmd, target.getColorImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, readback.buffer, 1,
                             &copy);
      VkMemoryBarrier2 host{VK_STRUCTURE_TYPE_MEMORY_BARRIER_2};
      host.srcStageMask = VK_PIPELINE_STAGE_2_COPY_BIT;
      host.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
      host.dstStageMask = VK_PIPELINE_STAGE_2_HOST_BIT;
      host.dstAccessMask = VK_ACCESS_2_HOST_READ_BIT;
      VkDependencyInfo dependency{VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
      dependency.memoryBarrierCount = 1;
      dependency.pMemoryBarriers = &host;
      vkCmdPipelineBarrier2(cmd, &dependency);
      barrier(cmd, target.getColorImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL,
              VK_PIPELINE_STAGE_2_COPY_BIT, VK_ACCESS_2_TRANSFER_READ_BIT, VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
              VK_ACCESS_2_MEMORY_READ_BIT);
    });
    void* p;
    vkCheck(vkMapMemory(c.device, readback.memory, 0, readback.size, 0, &p));
    capture(output, static_cast<uint8_t*>(p), int(size.width), int(size.height), false, mode >= 3);
    vkUnmapMemory(c.device, readback.memory);
  }
  void onDetach() override {
    vkDeviceWaitIdle(c.device);
    if (!smoke) display.deinit();
    scene.reset();
    if (uploadedVertices.buffer) allocator.destroyBuffer(uploadedVertices);
    target.deinit();
    allocator.deinit();
  }
};
inline int nvproMain(int argc, char** argv, int chapter) {
  bool smoke = false;
  for (int i = 1; i < argc; ++i) smoke |= std::strcmp(argv[i], "--smoke") == 0;
  int mode = chapter == 4 ? 6 : chapter == 6 ? 9 : chapter;
  nvShaders = std::filesystem::absolute(argv[0]).parent_path() / "shaders";
  NvContextOwner owner;
  auto& context = owner.context;
  nvValidationErrors = 0;
  nvvk::ContextInitInfo setup{};
  setup.instanceExtensions = {VK_EXT_DEBUG_UTILS_EXTENSION_NAME};
  setup.queues = {VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT};
  setup.enableValidationLayers = true;
  setup.verbose = false;
  setup.applicationName = "nvpro_core2 Course";
  if (!smoke && chapter != 1 && chapter != 5) {
    nvvk::addSurfaceExtensions(setup.instanceExtensions);
    setup.deviceExtensions = {{VK_KHR_SWAPCHAIN_EXTENSION_NAME}};
  }
  vkCheck(context.init(setup));
  VkDebugUtilsMessengerCreateInfoEXT debug{VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
  debug.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
  debug.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
  debug.pfnUserCallback = nvValidation;
  vkCheck(vkCreateDebugUtilsMessengerEXT(context.getInstance(), &debug, nullptr, &owner.messenger));
  {
    auto q = context.getQueueInfo(0);
    VkContextRef c{context.getInstance(), context.getPhysicalDevice(), context.getDevice(), q.queue, q.familyIndex};
    if (chapter == 1) {
      VkPhysicalDeviceProperties p;
      vkGetPhysicalDeviceProperties(c.physical, &p);
      std::cout << "nvpro_core2 context: " << p.deviceName << '\n';
    } else if (chapter == 5)
      computeSquares(c, nvShaders);
    else {
      nvapp::ApplicationCreateInfo info;
      info.name = "nvpro_core2 Course";
      info.instance = c.instance;
      info.device = c.device;
      info.physicalDevice = c.physical;
      info.queues = context.getQueueInfos();
      info.windowSize = {960, 640};
      info.headless = smoke;
      info.headlessFrameCount = 4;
      nvapp::Application app;
      app.init(info);
      auto element = std::make_shared<NvLesson>(
          mode, smoke, std::filesystem::path(argv[0]).parent_path() / ("nvpro_" + std::to_string(chapter) + ".ppm"));
      app.addElement(element);
      app.run();
      app.deinit();
    }
  }
  std::cout << "Validation errors: " << nvValidationErrors << "\n";
  return nvValidationErrors ? 1 : 0;
}
}  // namespace course
