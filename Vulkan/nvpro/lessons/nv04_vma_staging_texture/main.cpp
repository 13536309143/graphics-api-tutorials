// nv04_vma_staging_texture
// 本目录拥有完整课程源码；只共享第三方依赖，不链接其他章节的课程实现。
#include "support/nvapp_course.hpp"
namespace course {
int runChapter(int argc, char** argv) {
    bool smoke = false;
    for (int i = 1; i < argc; ++i)
        smoke |= std::strcmp(argv[i], "--smoke") == 0;
    // 课程编号与公共场景模式分别管理：第 4 课看纹理上传，第 6 课看 PBR。
    int mode = (6);
    nvShaders = std::filesystem::absolute(argv[0]).parent_path() / COURSE_RUNTIME_NAME / "shaders";
    NvContextOwner owner;
    auto& context = owner.context;
    nvValidationErrors = 0;
    nvvk::ContextInitInfo setup{};
    setup.instanceExtensions = {VK_EXT_DEBUG_UTILS_EXTENSION_NAME};
    setup.queues = {VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT};
    setup.enableValidationLayers = true;
    setup.verbose = false;
    setup.applicationName = "nvpro_core2 Course";
    if (!smoke && 4 != 1 && 4 != 5) {
        nvvk::addSurfaceExtensions(setup.instanceExtensions);
        setup.deviceExtensions = {{VK_KHR_SWAPCHAIN_EXTENSION_NAME}};
    }
    vkCheck(context.init(setup));
    VkDebugUtilsMessengerCreateInfoEXT debug{
        VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
    debug.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    debug.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
    debug.pfnUserCallback = nvValidation;
    vkCheck(
        vkCreateDebugUtilsMessengerEXT(context.getInstance(), &debug, nullptr, &owner.messenger));
    {
        auto q = context.getQueueInfo(0);
        VkContextRef c{context.getInstance(),
                       context.getPhysicalDevice(),
                       context.getDevice(),
                       q.queue,
                       q.familyIndex};
        {
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
            auto element = std::make_shared<NvLesson>(mode,
                                                      smoke,
                                                      std::filesystem::path(argv[0]).parent_path() /
                                                          ("nvpro_" + std::to_string(4) + ".ppm"));
            app.addElement(element);
            app.run();
            app.deinit();
        }
    }
    std::cout << "Validation errors: " << nvValidationErrors << "\n";
    return nvValidationErrors ? 1 : 0;
}
} // namespace course

int main(int argc, char** argv) try {
    return course::runChapter(argc, argv);
} catch (const std::exception& error) {
    std::cerr << "ERROR: " << error.what() << '\n';
    return 1;
}
