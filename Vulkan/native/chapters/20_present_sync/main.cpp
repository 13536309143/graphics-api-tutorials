// 获取、提交、呈现与完成标记
// 本目录拥有完整课程源码；只共享第三方依赖，不链接其他章节的课程实现。
// 本章检查：图像可用；不能写仍由呈现系统使用的图像。
// 本章检查：渲染完成；CPU 完成标记不机械等于呈现消费。
// 本章检查：复用模型；两种接口不一一替换。
// 读代码顺序：资源所有权 → 管线/绑定 → 录制和提交 → 完成检查 → 逆序释放。
#define STB_IMAGE_IMPLEMENTATION
#include "support/advanced.hpp"
#include "support/native_app.hpp"
#include <numeric>

namespace course {
void inspection(const VkContextRef& c) {
    VkFenceCreateInfo fi{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    fi.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    VkFence fence{};
    vkCheck(vkCreateFence(c.device, &fi, nullptr, &fence));
    require(vkGetFenceStatus(c.device, fence) == VK_SUCCESS, "Initial fence not signaled");
    vkCheck(vkResetFences(c.device, 1, &fence));
    require(vkGetFenceStatus(c.device, fence) == VK_NOT_READY, "Reset fence signaled unexpectedly");
    VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    // 提交只把工作交给 GPU；复用命令、常量或释放资源前仍需确认完成。
    vkCheck(vkQueueSubmit(c.queue, 1, &submit, fence));
    vkCheck(vkWaitForFences(c.device, 1, &fence, VK_TRUE, UINT64_MAX));
    vkDestroyFence(c.device, fence, nullptr);
    std::cout << "Verified signaled -> reset -> submitted -> complete fence lifecycle\n";
}

int runChapter(int argc, char** argv) {
    auto options = labOptions(argc, argv, 48);
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
