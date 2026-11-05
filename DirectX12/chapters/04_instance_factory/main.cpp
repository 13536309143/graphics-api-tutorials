// 实例扩展与 DXGI 工厂
// 本目录拥有完整课程源码；只共享第三方依赖，不链接其他章节的课程实现。
// 本章检查：实例层需求；要求与平台和实际功能匹配。
// 本章检查：设备层需求；不可把扩展名放错层次。
// 本章检查：无窗口路径；compute 依赖设备，不依赖呈现。
// 读代码顺序：资源所有权 → 管线/绑定 → 录制和提交 → 完成检查 → 逆序释放。
#define STB_IMAGE_IMPLEMENTATION
#include "support/advanced.hpp"
#include "support/dx12_course.hpp"
#include <numeric>
#include <d3d11shader.h>

namespace course {
// 查询、诊断与字节检查有明确的结果；合法 INFO 消息可能被默认过滤，临时空过滤器只扩大接收范围。
void inspection(DxDevice& c, const LabOptions& options) {
    UINT count = 0;
    for (UINT i = 0;; ++i) {
        ComPtr<IDXGIAdapter1> adapter;
        auto hr = c.factory->EnumAdapters1(i, &adapter);
        if (hr == DXGI_ERROR_NOT_FOUND)
            break;
        dxCheck(hr);
        DXGI_ADAPTER_DESC1 d{};
        dxCheck(adapter->GetDesc1(&d));
        std::wcout << L"adapter=" << d.Description << L", bytes=" << d.DedicatedVideoMemory
                   << L'\n';
        ++count;
    }
    require(count > 0, "No DXGI adapters enumerated");
}

int runChapter(int argc, char** argv) {
    auto options = labOptions(argc, argv, 37);
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
