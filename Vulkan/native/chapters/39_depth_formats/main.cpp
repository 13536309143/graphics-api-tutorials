// 深度格式、用途与样本支持
// 本目录拥有完整课程源码；只共享第三方依赖，不链接其他章节的课程实现。
// 本章检查：深度支持；用途成立后才选择格式。
// 本章检查：模板条件；纯 depth 与 depth/stencil 不混同。
// 本章检查：多样本交集；pipeline/PSO 附件同时匹配。
// 读代码顺序：资源所有权 → 管线/绑定 → 录制和提交 → 完成检查 → 逆序释放。
#define STB_IMAGE_IMPLEMENTATION
#include "support/advanced.hpp"
#include "support/native_app.hpp"
#include <numeric>

namespace course {
void inspection(const VkContextRef& c) {
    for (auto format : {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D16_UNORM}) {
        VkFormatProperties p{};
        vkGetPhysicalDeviceFormatProperties(c.physical, format, &p);
        std::cout << "depth format=" << format << ", optimal features=" << p.optimalTilingFeatures
                  << '\n';
    }
}

int runChapter(int argc, char** argv) {
    auto options = labOptions(argc, argv, 46);
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
