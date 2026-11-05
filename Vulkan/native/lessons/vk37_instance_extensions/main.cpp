// 关联教材：工具链、SDK、依赖与可复现构建。
// 关键约束：CPU 工具链；编译目标、运行产物和实际路径对应。
// 关键约束：能力来源；头文件版本不升级 GPU 驱动。
// 关键约束：构建隔离；各自 build 缓存，不混用 source root。
// vk37_instance_extensions
// 本目录拥有完整课程源码；只共享第三方依赖，不链接其他章节的课程实现。
#define STB_IMAGE_IMPLEMENTATION
#include "support/advanced.hpp"
#include "support/native_app.hpp"
#include <numeric>

namespace course {
void inspection(const VkContextRef& c) {
    uint32_t version{};
    vkCheck(vkEnumerateInstanceVersion(&version));
    uint32_t count{};
    vkCheck(vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr));
    std::vector<VkExtensionProperties> extensions(count);
    vkCheck(vkEnumerateInstanceExtensionProperties(nullptr, &count, extensions.data()));
    bool found = false;
    for (const auto& e : extensions) {
        std::cout << e.extensionName << '\n';
        found |= std::strcmp(e.extensionName, VK_EXT_DEBUG_UTILS_EXTENSION_NAME) == 0;
    }
    require(found, "Debug utils extension missing");
    std::cout << "Loader version=" << VK_API_VERSION_MAJOR(version) << '.'
              << VK_API_VERSION_MINOR(version) << '\n';
}

int runChapter(int argc, char** argv) {
    auto options = labOptions(argc, argv, 37);
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
