// 深度格式、用途与样本支持
// 本目录拥有完整课程源码；只共享第三方依赖，不链接其他章节的课程实现。
// 本章检查：深度支持；用途成立后才选择格式。
// 本章检查：模板条件；纯 depth 与 depth/stencil 不混同。
// 本章检查：多样本交集；pipeline/PSO 附件同时匹配。
// 读代码顺序：资源所有权 → 管线/绑定 → 录制和提交 → 完成检查 → 逆序释放。
#define STB_IMAGE_IMPLEMENTATION
#include "support/advanced.hpp"
#include "support/dx12_course.hpp"
#include <numeric>
#include <d3d11shader.h>

namespace course {
// 查询、诊断与字节检查有明确的结果；合法 INFO 消息可能被默认过滤，临时空过滤器只扩大接收范围。
void inspection(DxDevice& c, const LabOptions& options) {
    for (auto format :
         {DXGI_FORMAT_D32_FLOAT, DXGI_FORMAT_D24_UNORM_S8_UINT, DXGI_FORMAT_D16_UNORM}) {
        D3D12_FEATURE_DATA_FORMAT_SUPPORT q{format};
        dxCheck(c.device->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &q, sizeof(q)));
        std::cout << "depth format=" << format << ", support1=" << q.Support1 << '\n';
    }
}

int runChapter(int argc, char** argv) {
    auto options = labOptions(argc, argv, 46);
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
