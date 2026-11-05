// 关联教材：命令录制、提交与可验证工作。
// 关键约束：录制范围；提交前处于合法可执行状态。
// 关键约束：提交动作；返回不代表 GPU 工作完成。
// 关键约束：结果完成；读取之前完成与可见性同时成立。
// dx12_queue_submit
// 本目录拥有完整课程源码；只共享第三方依赖，不链接其他章节的课程实现。
#define STB_IMAGE_IMPLEMENTATION
#include "support/advanced.hpp"
#include "support/dx12_course.hpp"
#include <numeric>
#include <d3d11shader.h>

namespace course {
int runChapter(int argc, char** argv) {
    auto options = labOptions(argc, argv, 12);
    DxDevice c(options.warp);
    {
        c.begin();
        dxCheck(c.cmd->Close());
        ID3D12CommandList* lists[] = {c.cmd.Get()};
        // 提交是异步的；Fence 完成值决定 allocator 与上传资源何时可复用。
        c.queue->ExecuteCommandLists(1, lists);
        c.wait();
        require(c.fence->GetCompletedValue() >= c.value, "Queue fence did not complete");
        std::cout << "Verified command list close/submit and fence completion\n";
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
