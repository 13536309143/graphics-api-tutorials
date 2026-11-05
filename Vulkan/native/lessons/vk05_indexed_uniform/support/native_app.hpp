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
    NativeDevice(GLFWwindow* window, bool timeline = false) {
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
            VkPhysicalDeviceVulkan12Features f12{
                VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES};
            f13.pNext = &f12;
            VkPhysicalDeviceFeatures2 f2{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
            f2.pNext = &f13;
            vkGetPhysicalDeviceFeatures2(p, &f2);
            if (!f13.dynamicRendering || !f13.synchronization2 ||
                (timeline && !f12.timelineSemaphore))
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
        VkPhysicalDeviceVulkan12Features f12{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES};
        f12.timelineSemaphore = timeline;
        if (timeline)
            f13.pNext = &f12;
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
struct NativeRenderOptions {
    const Geometry* mesh = nullptr;
    VkCullModeFlags cull = VK_CULL_MODE_NONE;
    bool redOnly = false;
    std::string captureStem;
    VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT;
    std::function<void(SceneVk&)> setup;
    std::function<void(SceneVk&, float)> update;
};

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
    VkImageResource depth, offscreen, multisample;
    NativeRenderOptions options;
    std::unique_ptr<SceneVk> scene;
    int mode;
    Swapchain(NativeDevice& d, GLFWwindow* w, int lesson, NativeRenderOptions settings = {})
        : device(d), window(w), options(std::move(settings)), mode(lesson) {
        recreate();
    }
    void cleanup() {
        vkDeviceWaitIdle(device.c.device);
        scene.reset();
        depth.reset();
        multisample.reset();
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
                   VK_IMAGE_ASPECT_DEPTH_BIT,
                   1,
                   options.samples);
        if (options.samples != VK_SAMPLE_COUNT_1_BIT)
            multisample.init(device.c,
                             extent,
                             format,
                             VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                             VK_IMAGE_ASPECT_COLOR_BIT,
                             1,
                             options.samples);
        if (mode == 10)
            offscreen.init(device.c,
                           extent,
                           VK_FORMAT_R8G8B8A8_UNORM,
                           VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
                           VK_IMAGE_ASPECT_COLOR_BIT);
        if (mode >= 3)
            scene = std::make_unique<SceneVk>(device.c,
                                              mode,
                                              mode == 10 ? VK_FORMAT_R8G8B8A8_UNORM : format,
                                              nativeShaders,
                                              options.mesh,
                                              options.samples,
                                              VK_NULL_HANDLE,
                                              options.cull,
                                              options.redOnly);
        if (scene && options.setup)
            options.setup(*scene);
    }
    ~Swapchain() {
        cleanup();
    }
};

} // namespace course
