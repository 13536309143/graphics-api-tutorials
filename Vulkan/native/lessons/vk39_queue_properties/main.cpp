// 关联教材：队列族、命令队列与并行边界。
// 关键约束：队列选择；命令必须被支持该操作的队列执行。
// 关键约束：命令存储归属；不能提交不兼容命令。
// 关键约束：并行能力；实际重叠需要计时证据。
// vk39_queue_properties
// 本目录拥有完整课程源码；只共享第三方依赖，不链接其他章节的课程实现。
#define STB_IMAGE_IMPLEMENTATION
#include "support/advanced.hpp"
#include "support/native_app.hpp"
#include <numeric>

namespace course {
void inspection(const VkContextRef& c) {
    uint32_t count{};
    vkGetPhysicalDeviceQueueFamilyProperties(c.physical, &count, nullptr);
    std::vector<VkQueueFamilyProperties> queues(count);
    vkGetPhysicalDeviceQueueFamilyProperties(c.physical, &count, queues.data());
    for (uint32_t i = 0; i < count; ++i)
        std::cout << "family=" << i << ", queues=" << queues[i].queueCount
                  << ", flags=" << queues[i].queueFlags
                  << ", timestampBits=" << queues[i].timestampValidBits << '\n';
    require((queues[c.family].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0,
            "Selected family lacks graphics");
}

int runChapter(int argc, char** argv) {
    auto options = labOptions(argc, argv, 39);
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
