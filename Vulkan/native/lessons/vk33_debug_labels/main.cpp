// 关联教材：验证层、调试队列与诊断证据。
// 关键约束：诊断创建时机；早期创建错误也能被观察。
// 关键约束：合法测试消息；信息投递不等于发生 API 错误。
// 关键约束：GPU 调试命名；不直接伪造工具事件载荷。
// vk33_debug_labels
// 本目录拥有完整课程源码；只共享第三方依赖，不链接其他章节的课程实现。
#define STB_IMAGE_IMPLEMENTATION
#include "support/advanced.hpp"
#include "support/native_app.hpp"
#include <numeric>

namespace course {
int runChapter(int argc, char** argv) {
    auto options = labOptions(argc, argv, 33);
    nativeShaders = options.directory / "shaders";
    validationErrors = 0;
    {
        NativeDevice owner(nullptr, false);
        const auto& c = owner.c;
        {
            auto begin = reinterpret_cast<PFN_vkCmdBeginDebugUtilsLabelEXT>(
                vkGetDeviceProcAddr(c.device, "vkCmdBeginDebugUtilsLabelEXT"));
            auto end = reinterpret_cast<PFN_vkCmdEndDebugUtilsLabelEXT>(
                vkGetDeviceProcAddr(c.device, "vkCmdEndDebugUtilsLabelEXT"));
            require(begin && end, "Debug label entry points missing");
            immediate(c, [&](VkCommandBuffer cmd) {
                VkDebugUtilsLabelEXT label{VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT};
                label.pLabelName = "Course/Upload pass";
                label.color[1] = 1;
                begin(cmd, &label);
                end(cmd);
            });
            std::cout << "Submitted named GPU debug region\n";
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
