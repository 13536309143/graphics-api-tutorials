// 关联教材：验证层、调试队列与诊断证据。
// 关键约束：诊断创建时机；早期创建错误也能被观察。
// 关键约束：合法测试消息；信息投递不等于发生 API 错误。
// 关键约束：GPU 调试命名；不直接伪造工具事件载荷。
// vk38_diagnostic_delivery
// 本目录拥有完整课程源码；只共享第三方依赖，不链接其他章节的课程实现。
#define STB_IMAGE_IMPLEMENTATION
#include "support/advanced.hpp"
#include "support/native_app.hpp"
#include <numeric>

namespace course {
void inspection(const VkContextRef& c) {
    uint32_t layers{};
    vkCheck(vkEnumerateInstanceLayerProperties(&layers, nullptr));
    std::vector<VkLayerProperties> list(layers);
    vkCheck(vkEnumerateInstanceLayerProperties(&layers, list.data()));
    for (const auto& l : list)
        std::cout << "layer=" << l.layerName << '\n';
    int delivered = 0;
    VkDebugUtilsMessengerCreateInfoEXT info{
        VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
    info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;
    info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT;
    info.pUserData = &delivered;
    info.pfnUserCallback = [](VkDebugUtilsMessageSeverityFlagBitsEXT,
                              VkDebugUtilsMessageTypeFlagsEXT,
                              const VkDebugUtilsMessengerCallbackDataEXT*,
                              void* user) -> VkBool32 {
        ++*static_cast<int*>(user);
        return VK_FALSE;
    };
    auto create = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
        vkGetInstanceProcAddr(c.instance, "vkCreateDebugUtilsMessengerEXT"));
    auto destroy = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
        vkGetInstanceProcAddr(c.instance, "vkDestroyDebugUtilsMessengerEXT"));
    auto message = reinterpret_cast<PFN_vkSubmitDebugUtilsMessageEXT>(
        vkGetInstanceProcAddr(c.instance, "vkSubmitDebugUtilsMessageEXT"));
    require(create && destroy && message, "Debug utils functions missing");
    VkDebugUtilsMessengerEXT messenger{};
    vkCheck(create(c.instance, &info, nullptr, &messenger));
    VkDebugUtilsMessengerCallbackDataEXT data{
        VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CALLBACK_DATA_EXT};
    data.pMessage = "Course informational diagnostic";
    message(c.instance,
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT,
            VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT,
            &data);
    destroy(c.instance, messenger, nullptr);
    require(delivered > 0, "Diagnostic callback was not invoked");
    std::cout << "Verified application informational message delivery\n";
}

int runChapter(int argc, char** argv) {
    auto options = labOptions(argc, argv, 38);
    nativeShaders = options.directory / "shaders";
    validationErrors = 0;
    {
        NativeDevice owner(nullptr, false);
        const auto& c = owner.c;
        {
            inspection(c);
        }
    }
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
