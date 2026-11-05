// 验证层、调试队列与诊断证据
// 本目录拥有完整课程源码；只共享第三方依赖，不链接其他章节的课程实现。
// 本章检查：诊断创建时机；早期创建错误也能被观察。
// 本章检查：合法测试消息；信息投递不等于发生 API 错误。
// 本章检查：GPU 调试命名；不直接伪造工具事件载荷。
// 读代码顺序：资源所有权 → 管线/绑定 → 录制和提交 → 完成检查 → 逆序释放。
#define STB_IMAGE_IMPLEMENTATION
#include "support/advanced.hpp"
#include "support/dx12_course.hpp"
#include <numeric>
#include <d3d11shader.h>

namespace course {
// 查询、诊断与字节检查有明确的结果；合法 INFO 消息可能被默认过滤，临时空过滤器只扩大接收范围。
void inspection(DxDevice& c, const LabOptions& options) {
    require(c.messages.Get() != nullptr, "D3D12 InfoQueue unavailable");
    dxCheck(c.messages->PushEmptyStorageFilter());
    UINT64 before = c.messages->GetNumStoredMessages();
    dxCheck(c.messages->AddApplicationMessage(D3D12_MESSAGE_SEVERITY_INFO,
                                              "Course informational diagnostic"));
    require(c.messages->GetNumStoredMessages() > before,
            "InfoQueue did not receive application message");
    c.messages->PopStorageFilter();
    std::cout
        << "Verified informational diagnostic delivery without introducing invalid API usage\n";
}

int runChapter(int argc, char** argv) {
    auto options = labOptions(argc, argv, 38);
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
