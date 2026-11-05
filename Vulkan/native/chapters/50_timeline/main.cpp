// Timeline、Fence 计数与生产消费链
// 本目录拥有完整课程源码；只共享第三方依赖，不链接其他章节的课程实现。
// 本章检查：GPU 链；值对应真实执行进度。
// 本章检查：CPU 等待；与 GPU 等待不是同一阻塞主体。
// 本章检查：范围边界；不作为独立硬件引擎并行证据。
// 读代码顺序：资源所有权 → 管线/绑定 → 录制和提交 → 完成检查 → 逆序释放。
#define STB_IMAGE_IMPLEMENTATION
#include "support/advanced.hpp"
#include "support/native_app.hpp"
#include <numeric>

namespace course {
// GPU 先 signal 1，下一提交 wait 1 / signal 2；CPU 等 2 后才能宣告链路完成。
void timelineCounter(const VkContextRef& c) {
    VkSemaphoreTypeCreateInfo type{VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO};
    type.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
    VkSemaphoreCreateInfo info{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    info.pNext = &type;
    VkSemaphore semaphore{};
    vkCheck(vkCreateSemaphore(c.device, &info, nullptr, &semaphore));
    VkSemaphoreSubmitInfo signal{VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO};
    signal.semaphore = semaphore;
    signal.value = 1;
    signal.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
    VkSubmitInfo2 first{VK_STRUCTURE_TYPE_SUBMIT_INFO_2};
    first.signalSemaphoreInfoCount = 1;
    first.pSignalSemaphoreInfos = &signal;
    vkCheck(vkQueueSubmit2(c.queue, 1, &first, VK_NULL_HANDLE));
    VkSemaphoreSubmitInfo wait = signal;
    signal.value = 2;
    VkSubmitInfo2 second{VK_STRUCTURE_TYPE_SUBMIT_INFO_2};
    second.waitSemaphoreInfoCount = 1;
    second.pWaitSemaphoreInfos = &wait;
    second.signalSemaphoreInfoCount = 1;
    second.pSignalSemaphoreInfos = &signal;
    vkCheck(vkQueueSubmit2(c.queue, 1, &second, VK_NULL_HANDLE));
    uint64_t target = 2;
    VkSemaphoreWaitInfo cpu{VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO};
    cpu.semaphoreCount = 1;
    cpu.pSemaphores = &semaphore;
    cpu.pValues = &target;
    vkCheck(vkWaitSemaphores(c.device, &cpu, UINT64_MAX));
    uint64_t counter{};
    vkCheck(vkGetSemaphoreCounterValue(c.device, semaphore, &counter));
    require(counter >= 2, "Timeline did not advance");
    vkCheck(vkQueueWaitIdle(c.queue));
    vkDestroySemaphore(c.device, semaphore, nullptr);
    std::cout << "Verified timeline producer 1 -> consumer 2, counter=" << counter << '\n';
}

int runChapter(int argc, char** argv) {
    auto options = labOptions(argc, argv, 26);
    nativeShaders = options.directory / "shaders";
    validationErrors = 0;
    {
        NativeDevice owner(nullptr, true);
        const auto& c = owner.c;
        {
            timelineCounter(c);
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
