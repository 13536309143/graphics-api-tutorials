// 关联教材：验证层、调试队列与诊断证据。
// 关键约束：诊断创建时机；早期创建错误也能被观察。
// 关键约束：合法测试消息；信息投递不等于发生 API 错误。
// 关键约束：GPU 调试命名；不直接伪造工具事件载荷。
// dx33_debug_labels
// 本目录拥有完整课程源码；只共享第三方依赖，不链接其他章节的课程实现。
#define STB_IMAGE_IMPLEMENTATION
#include "support/advanced.hpp"
#include "support/dx12_course.hpp"
#include <numeric>
#include <d3d11shader.h>

namespace course {
int runChapter(int argc, char** argv) {
    auto options = labOptions(argc, argv, 33);
    DxDevice c(options.warp);
    {
        dxCheck(c.queue->SetName(L"Course Graphics Queue"));
        dxCheck(c.cmd->SetName(L"Course Upload Commands"));
        dxCheck(c.allocator->SetName(L"Course Upload Allocator"));
        c.begin();
        c.submit();
        std::cout << "Named queue, command list and allocator for GPU debugging; event regions "
                     "require PIX helpers\n";
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
