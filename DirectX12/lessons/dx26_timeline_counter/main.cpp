// 关联教材：Timeline、Fence 计数与生产消费链。
// 关键约束：GPU 链；值对应真实执行进度。
// 关键约束：CPU 等待；与 GPU 等待不是同一阻塞主体。
// 关键约束：范围边界；不作为独立硬件引擎并行证据。
// dx26_timeline_counter
// 本目录拥有完整课程源码；只共享第三方依赖，不链接其他章节的课程实现。
#define STB_IMAGE_IMPLEMENTATION
#include "support/advanced.hpp"
#include "support/dx12_course.hpp"
#include <numeric>
#include <d3d11shader.h>

namespace course {
// fence 单调完成值保护复用与回收，不采用 Vulkan binary fence 的 reset 模型。
void waitValue(DxDevice& c, UINT64 value) {
    if (c.fence->GetCompletedValue() < value) {
        dxCheck(c.fence->SetEventOnCompletion(value, c.event));
        require(WaitForSingleObject(c.event, 30000) == WAIT_OBJECT_0, "Fence timeout");
    }
}

int runChapter(int argc, char** argv) {
    auto options = labOptions(argc, argv, 26);
    DxDevice c(options.warp);
    {
        dxCheck(c.queue->Signal(c.fence.Get(), ++c.value));
        dxCheck(c.queue->Wait(c.fence.Get(), c.value));
        dxCheck(c.queue->Signal(c.fence.Get(), ++c.value));
        waitValue(c, c.value);
        require(c.fence->GetCompletedValue() >= 2, "Fence counter did not advance");
        std::cout << "Verified queue producer 1 -> consumer 2 fence chain\n";
    }
    c.checkMessages();
    return 0;
}
} // namespace course

int main(int argc, char** argv) try {
    return course::runChapter(argc, argv);
} catch (const std::exception& error) {
    std::cerr << "ERROR: " << error.what() << '\n';
    return 1;
}
