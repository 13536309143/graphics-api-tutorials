// 实例扩展与 DXGI 工厂
// 本目录拥有完整课程源码；只共享第三方依赖，不链接其他章节的课程实现。
// 本章检查：实例层需求；要求与平台和实际功能匹配。
// 本章检查：设备层需求；不可把扩展名放错层次。
// 本章检查：无窗口路径；compute 依赖设备，不依赖呈现。
// 读代码顺序：资源所有权 → 管线/绑定 → 录制和提交 → 完成检查 → 逆序释放。
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
