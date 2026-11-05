// 关联教材：队列族、命令队列与并行边界。
// 关键约束：队列选择；命令必须被支持该操作的队列执行。
// 关键约束：命令存储归属；不能提交不兼容命令。
// 关键约束：并行能力；实际重叠需要计时证据。
// dx39_queue_properties
// 本目录拥有完整课程源码；只共享第三方依赖，不链接其他章节的课程实现。
#define STB_IMAGE_IMPLEMENTATION
#include "support/advanced.hpp"
#include "support/dx12_course.hpp"
#include <numeric>
#include <d3d11shader.h>

namespace course {
// 查询、诊断与字节检查有明确的结果；合法 INFO 消息可能被默认过滤，临时空过滤器只扩大接收范围。
void inspection(DxDevice& c, const LabOptions& options) {
    auto desc = c.queue->GetDesc();
    require(desc.Type == D3D12_COMMAND_LIST_TYPE_DIRECT, "Expected direct queue");
    std::cout << "queue type=" << desc.Type << ", priority=" << desc.Priority
              << ", flags=" << desc.Flags << ", nodeMask=" << desc.NodeMask << '\n';
}

int runChapter(int argc, char** argv) {
    auto options = labOptions(argc, argv, 39);
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
