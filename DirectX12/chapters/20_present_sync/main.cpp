// 获取、提交、呈现与完成标记
// 本目录拥有完整课程源码；只共享第三方依赖，不链接其他章节的课程实现。
// 本章检查：图像可用；不能写仍由呈现系统使用的图像。
// 本章检查：渲染完成；CPU 完成标记不机械等于呈现消费。
// 本章检查：复用模型；两种接口不一一替换。
// 读代码顺序：资源所有权 → 管线/绑定 → 录制和提交 → 完成检查 → 逆序释放。
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

// 查询、诊断与字节检查有明确的结果；合法 INFO 消息可能被默认过滤，临时空过滤器只扩大接收范围。
void inspection(DxDevice& c, const LabOptions& options) {
    auto initial = c.fence->GetCompletedValue();
    dxCheck(c.queue->Signal(c.fence.Get(), ++c.value));
    waitValue(c, c.value);
    require(c.fence->GetCompletedValue() > initial, "Fence counter failed to advance");
    dxCheck(c.queue->Signal(c.fence.Get(), ++c.value));
    waitValue(c, c.value);
    std::cout << "Verified monotonic fence reuse; DX12 fence is not reset like a binary Vulkan "
                 "fence\n";
}

int runChapter(int argc, char** argv) {
    auto options = labOptions(argc, argv, 48);
    DxDevice c(options.warp);
    {
        inspection(c, options);
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
