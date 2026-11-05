// 关联教材：内存类型、Heap 与分配要求。
// 关键约束：逻辑与实际大小；实际分配可能大于请求。
// 关键约束：选择约束；不能仅按类型编号猜位置。
// 关键约束：接口对齐；不同于一般 allocation alignment。
// dx47_allocation_requirements
// 本目录拥有完整课程源码；只共享第三方依赖，不链接其他章节的课程实现。
#define STB_IMAGE_IMPLEMENTATION
#include "support/advanced.hpp"
#include "support/dx12_course.hpp"
#include <numeric>
#include <d3d11shader.h>

namespace course {
// 查询、诊断与字节检查有明确的结果；合法 INFO 消息可能被默认过滤，临时空过滤器只扩大接收范围。
void inspection(DxDevice& c, const LabOptions& options) {
    D3D12_RESOURCE_DESC desc{};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    desc.Width = 257;
    desc.Height = 1;
    desc.DepthOrArraySize = 1;
    desc.MipLevels = 1;
    desc.SampleDesc.Count = 1;
    desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    auto allocation = c.device->GetResourceAllocationInfo(0, 1, &desc);
    require(allocation.SizeInBytes >= 257, "Allocation too small");
    auto upload = c.device->GetCustomHeapProperties(0, D3D12_HEAP_TYPE_UPLOAD);
    std::cout << "requested=257, allocation=" << allocation.SizeInBytes
              << ", alignment=" << allocation.Alignment
              << ", CPUPageProperty=" << upload.CPUPageProperty << '\n';
}

int runChapter(int argc, char** argv) {
    auto options = labOptions(argc, argv, 47);
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
