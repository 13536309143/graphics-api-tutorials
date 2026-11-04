#pragma once
#include <GLFW/glfw3.h>
#include "scene_vk.hpp"
#include <atomic>
#include <memory>
namespace course {
inline std::atomic<int> validationErrors = 0;
inline std::filesystem::path nativeShaders;
inline VKAPI_ATTR VkBool32 VKAPI_CALL validation(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
                                                 VkDebugUtilsMessageTypeFlagsEXT type,
                                                 const VkDebugUtilsMessengerCallbackDataEXT* data,
                                                 void*) {
    if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
        if (type & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT) {
            ++validationErrors;
            std::cerr << "VALIDATION ERROR: ";
        } else
            std::cerr << "VULKAN LOADER DIAGNOSTIC: ";
        std::cerr << data->pMessage << '\n';
    }
    return VK_FALSE;
}

// 手动创建 Instance → PhysicalDevice/queue family → Device/Queue；销毁时反向处理。
struct NativeDevice {
    VkContextRef c;
    VkDebugUtilsMessengerEXT messenger{};
    VkSurfaceKHR surface{};
    NativeDevice(GLFWwindow* window) {
        uint32_t count = 0;
        std::vector<const char*> extensions;
        if (window) {
            const char** required = glfwGetRequiredInstanceExtensions(&count);
            extensions.assign(required, required + count);
        }
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        uint32_t layerCount = 0;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
        std::vector<VkLayerProperties> layers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, layers.data());
        bool debug = std::any_of(layers.begin(), layers.end(), [](const auto& x) {
            return std::strcmp(x.layerName, "VK_LAYER_KHRONOS_validation") == 0;
        });
        const char* layer = "VK_LAYER_KHRONOS_validation";
        if (!debug)
            std::cerr << "Validation layer not installed; rendering remains available\n";
        VkApplicationInfo app{VK_STRUCTURE_TYPE_APPLICATION_INFO};
        app.pApplicationName = "Native Vulkan Course";
        app.apiVersion = VK_API_VERSION_1_3;
        VkDebugUtilsMessengerCreateInfoEXT di{
            VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
        di.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
        di.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                         VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                         VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        di.pfnUserCallback = validation;
        VkInstanceCreateInfo info{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
        info.pApplicationInfo = &app;
        info.enabledExtensionCount = uint32_t(extensions.size());
        info.ppEnabledExtensionNames = extensions.data();
        if (debug) {
            info.enabledLayerCount = 1;
            info.ppEnabledLayerNames = &layer;
            info.pNext = &di;
        }
        vkCheck(vkCreateInstance(&info, nullptr, &c.instance));
        auto create = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
            vkGetInstanceProcAddr(c.instance, "vkCreateDebugUtilsMessengerEXT"));
        if (debug && create)
            vkCheck(create(c.instance, &di, nullptr, &messenger));
        if (window)
            vkCheck(glfwCreateWindowSurface(c.instance, window, nullptr, &surface));
        vkCheck(vkEnumeratePhysicalDevices(c.instance, &count, nullptr));
        std::vector<VkPhysicalDevice> devices(count);
        vkCheck(vkEnumeratePhysicalDevices(c.instance, &count, devices.data()));

        // 选择设备时同时确认 API 版本、所需特性以及同一队列族的图形/计算/呈现能力。
        for (auto p : devices) {
            VkPhysicalDeviceProperties props;
            vkGetPhysicalDeviceProperties(p, &props);
            if (props.apiVersion < VK_API_VERSION_1_3)
                continue;
            VkPhysicalDeviceVulkan13Features f13{
                VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
            VkPhysicalDeviceFeatures2 f2{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
            f2.pNext = &f13;
            vkGetPhysicalDeviceFeatures2(p, &f2);
            if (!f13.dynamicRendering || !f13.synchronization2)
                continue;
            uint32_t n;
            vkGetPhysicalDeviceQueueFamilyProperties(p, &n, nullptr);
            std::vector<VkQueueFamilyProperties> families(n);
            vkGetPhysicalDeviceQueueFamilyProperties(p, &n, families.data());
            for (uint32_t i = 0; i < n; ++i) {
                VkBool32 present = VK_TRUE;
                if (surface)
                    vkCheck(vkGetPhysicalDeviceSurfaceSupportKHR(p, i, surface, &present));
                if ((families[i].queueFlags & (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT)) ==
                        (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT) &&
                    present) {
                    c.physical = p;
                    c.family = i;
                    std::cout << "GPU: " << props.deviceName << ", Vulkan "
                              << VK_API_VERSION_MAJOR(props.apiVersion) << '.'
                              << VK_API_VERSION_MINOR(props.apiVersion) << '\n';
                    break;
                }
            }
            if (c.physical)
                break;
        }
        if (!c.physical)
            throw std::runtime_error("Vulkan 1.3 graphics+compute+present device not found");
        float priority = 1;
        VkDeviceQueueCreateInfo qi{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
        qi.queueFamilyIndex = c.family;
        qi.queueCount = 1;
        qi.pQueuePriorities = &priority;
        VkPhysicalDeviceVulkan13Features f13{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
        f13.dynamicRendering = VK_TRUE;
        f13.synchronization2 = VK_TRUE;
        VkDeviceCreateInfo device{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
        device.pNext = &f13;
        device.queueCreateInfoCount = 1;
        device.pQueueCreateInfos = &qi;
        const char* extension = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
        if (surface) {
            device.enabledExtensionCount = 1;
            device.ppEnabledExtensionNames = &extension;
        }
        vkCheck(vkCreateDevice(c.physical, &device, nullptr, &c.device));
        vkGetDeviceQueue(c.device, c.family, 0, &c.queue);
    }
    ~NativeDevice() {
        if (c.device) {
            vkDeviceWaitIdle(c.device);
            vkDestroyDevice(c.device, nullptr);
        }
        if (surface)
            vkDestroySurfaceKHR(c.instance, surface, nullptr);
        if (messenger) {
            auto fn = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
                vkGetInstanceProcAddr(c.instance, "vkDestroyDebugUtilsMessengerEXT"));
            fn(c.instance, messenger, nullptr);
        }
        if (c.instance)
            vkDestroyInstance(c.instance, nullptr);
    }
};

// 交换链图像由实现分配；应用拥有它们的 image view、深度附件和呈现同步对象。
struct Swapchain {
    NativeDevice& device;
    GLFWwindow* window;
    VkSwapchainKHR handle{};
    VkFormat format{};
    VkExtent2D extent{};
    std::vector<VkImage> images;
    std::vector<VkImageView> views;
    // 每张交换链图像拥有独立的 present-wait 信号量。
    std::vector<VkSemaphore> presentReady;
    VkImageResource depth, offscreen;
    std::unique_ptr<SceneVk> scene;
    int mode;
    Swapchain(NativeDevice& d, GLFWwindow* w, int lesson) : device(d), window(w), mode(lesson) {
        recreate();
    }
    void cleanup() {
        vkDeviceWaitIdle(device.c.device);
        scene.reset();
        depth.reset();
        offscreen.reset();
        for (auto v : views)
            vkDestroyImageView(device.c.device, v, nullptr);
        views.clear();
        for (auto semaphore : presentReady)
            vkDestroySemaphore(device.c.device, semaphore, nullptr);
        presentReady.clear();
        if (handle)
            vkDestroySwapchainKHR(device.c.device, handle, nullptr);
        handle = {};
    }

    // 重建前等待 GPU 并释放旧附件；窗口大小变化后重新读取 surface 能力和 extent。
    void recreate() {
        cleanup();
        VkSurfaceCapabilitiesKHR caps;
        vkCheck(
            vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device.c.physical, device.surface, &caps));
        uint32_t count;
        vkCheck(vkGetPhysicalDeviceSurfaceFormatsKHR(
            device.c.physical, device.surface, &count, nullptr));
        std::vector<VkSurfaceFormatKHR> formats(count);
        vkCheck(vkGetPhysicalDeviceSurfaceFormatsKHR(
            device.c.physical, device.surface, &count, formats.data()));
        auto selected = formats[0];
        for (auto f : formats)
            if (f.format == VK_FORMAT_B8G8R8A8_UNORM &&
                f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
                selected = f;
        format = selected.format;
        extent = caps.currentExtent;
        if (extent.width == UINT32_MAX) {
            int w, h;
            glfwGetFramebufferSize(window, &w, &h);
            extent = {
                std::clamp(uint32_t(w), caps.minImageExtent.width, caps.maxImageExtent.width),
                std::clamp(uint32_t(h), caps.minImageExtent.height, caps.maxImageExtent.height)};
        }
        VkImageUsageFlags usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                                  VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        if ((caps.supportedUsageFlags & usage) != usage)
            throw std::runtime_error("Surface does not support transfer readback/blit");
        VkSwapchainCreateInfoKHR si{VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
        si.surface = device.surface;
        si.minImageCount = caps.minImageCount + 1;
        if (caps.maxImageCount)
            si.minImageCount = std::min(si.minImageCount, caps.maxImageCount);
        si.imageFormat = format;
        si.imageColorSpace = selected.colorSpace;
        si.imageExtent = extent;
        si.imageArrayLayers = 1;
        si.imageUsage = usage;
        si.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        si.preTransform = caps.currentTransform;
        si.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        si.presentMode = VK_PRESENT_MODE_FIFO_KHR;
        si.clipped = VK_TRUE;
        vkCheck(vkCreateSwapchainKHR(device.c.device, &si, nullptr, &handle));
        vkGetSwapchainImagesKHR(device.c.device, handle, &count, nullptr);
        images.resize(count);
        vkGetSwapchainImagesKHR(device.c.device, handle, &count, images.data());
        for (auto image : images) {
            VkImageViewCreateInfo vi{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
            vi.image = image;
            vi.viewType = VK_IMAGE_VIEW_TYPE_2D;
            vi.format = format;
            vi.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
            VkImageView view;
            vkCheck(vkCreateImageView(device.c.device, &vi, nullptr, &view));
            views.push_back(view);
            VkSemaphoreCreateInfo sem{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
            VkSemaphore ready;
            vkCheck(vkCreateSemaphore(device.c.device, &sem, nullptr, &ready));
            presentReady.push_back(ready);
        }
        depth.init(device.c,
                   extent,
                   VK_FORMAT_D32_SFLOAT,
                   VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                   VK_IMAGE_ASPECT_DEPTH_BIT);
        if (mode == 10)
            offscreen.init(device.c,
                           extent,
                           VK_FORMAT_R8G8B8A8_UNORM,
                           VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
                           VK_IMAGE_ASPECT_COLOR_BIT);
        if (mode >= 3)
            scene = std::make_unique<SceneVk>(
                device.c, mode, mode == 10 ? VK_FORMAT_R8G8B8A8_UNORM : format, nativeShaders);
    }
    ~Swapchain() {
        cleanup();
    }
};
inline int nativeMain(int argc, char** argv, int mode) {
    bool smoke = false;
    for (int i = 1; i < argc; ++i)
        smoke |= std::strcmp(argv[i], "--smoke") == 0;
    validationErrors = 0;
    nativeShaders = std::filesystem::absolute(argv[0]).parent_path() / "shaders";

    // 设备探测和计算课不创建窗口，也不需要 surface 或交换链。
    if (mode == 1 || mode == 8) {
        NativeDevice d(nullptr);
        if (mode == 8)
            computeSquares(d.c, nativeShaders);
        return validationErrors ? 1 : 0;
    }
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
        Swapchain swap(d, window, mode);
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
                    VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
                    VK_PIPELINE_STAGE_2_NONE,
                    0,
                    VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT |
                        VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
                    VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                    VK_IMAGE_ASPECT_DEPTH_BIT);
            VkRenderingAttachmentInfo color{VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
            color.imageView = targetView;
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
            if (swap.scene)
                swap.scene->render(cmd, swap.extent, smoke ? 1.f : float(glfwGetTime()));
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
                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                        VK_PIPELINE_STAGE_2_NONE,
                        0,
                        VK_PIPELINE_STAGE_2_BLIT_BIT,
                        VK_ACCESS_2_TRANSFER_WRITE_BIT);
                VkImageBlit blit{};
                blit.srcSubresource = blit.dstSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
                blit.srcOffsets[1] = blit.dstOffsets[1] = {
                    int32_t(swap.extent.width), int32_t(swap.extent.height), 1};
                vkCmdBlitImage(cmd,
                               target,
                               VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                               swap.images[imageIndex],
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                               1,
                               &blit,
                               VK_FILTER_NEAREST);
            }
            bool captureFrame = smoke && frames == 3;
            std::unique_ptr<VkBufferResource> readback;
            if (captureFrame) {
                readback = std::make_unique<VkBufferResource>();
                readback->init(d.c,
                               VkDeviceSize(swap.extent.width) * swap.extent.height * 4,
                               VK_BUFFER_USAGE_TRANSFER_DST_BIT);
                barrier(cmd,
                        swap.images[imageIndex],
                        mode == 10 ? VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
                                   : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                        mode == 10 ? VK_PIPELINE_STAGE_2_BLIT_BIT
                                   : VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                        mode == 10 ? VK_ACCESS_2_TRANSFER_WRITE_BIT
                                   : VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
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
                vkCmdPipelineBarrier2(cmd, &dep);
            }
            barrier(cmd,
                    swap.images[imageIndex],
                    captureFrame ? VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
                    : mode == 10 ? VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
                                 : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                    VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                    captureFrame ? VK_PIPELINE_STAGE_2_COPY_BIT
                    : mode == 10 ? VK_PIPELINE_STAGE_2_BLIT_BIT
                                 : VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                    captureFrame ? VK_ACCESS_2_TRANSFER_READ_BIT
                    : mode == 10 ? VK_ACCESS_2_TRANSFER_WRITE_BIT
                                 : VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
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
                            ("native_" + std::to_string(mode) + ".ppm"),
                        static_cast<uint8_t*>(p),
                        int(swap.extent.width),
                        int(swap.extent.height),
                        swap.format == VK_FORMAT_B8G8R8A8_UNORM ||
                            swap.format == VK_FORMAT_B8G8R8A8_SRGB,
                        mode >= 3);
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
