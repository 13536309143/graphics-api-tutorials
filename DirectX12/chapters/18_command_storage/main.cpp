// 命令池、Allocator 与安全重置
// 本目录拥有完整课程源码；只共享第三方依赖，不链接其他章节的课程实现。
// 本章检查：命令使用期；Reset 的安全条件来自 GPU 进度。
// 本章检查：重置粒度；对象与存储的规则分别核对。
// 本章检查：四代验证；每代核对，避免只看最后一代。
// 读代码顺序：资源所有权 → 管线/绑定 → 录制和提交 → 完成检查 → 逆序释放。
#define STB_IMAGE_IMPLEMENTATION
#include "support/advanced.hpp"
#include "support/dx12_course.hpp"
#include <numeric>
#include <d3d11shader.h>

namespace course {
void writeBuffer(ID3D12Resource* resource, const void* data, size_t bytes) {
    void* p{};
    D3D12_RANGE empty{0, 0};
    dxCheck(resource->Map(0, &empty, &p));
    std::memcpy(p, data, bytes);
    resource->Unmap(0, nullptr);
}

std::vector<uint32_t> readBuffer(ID3D12Resource* resource, size_t count) {
    void* p{};
    D3D12_RANGE range{0, count * 4};
    dxCheck(resource->Map(0, &range, &p));
    std::vector<uint32_t> result(static_cast<uint32_t*>(p), static_cast<uint32_t*>(p) + count);
    D3D12_RANGE empty{0, 0};
    resource->Unmap(0, &empty);
    return result;
}

// 查询、诊断与字节检查有明确的结果；合法 INFO 消息可能被默认过滤，临时空过滤器只扩大接收范围。
void inspection(DxDevice& c, const LabOptions& options) {
    auto upload = c.buffer(256, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ);
    auto readback = c.buffer(256, D3D12_HEAP_TYPE_READBACK, D3D12_RESOURCE_STATE_COPY_DEST);
    for (uint32_t generation = 0; generation < 4; ++generation) {
        std::vector<uint32_t> values(64, 41 + generation);
        writeBuffer(upload.Get(), values.data(), 256);
        c.begin();
        c.cmd->CopyBufferRegion(readback.Get(), 0, upload.Get(), 0, 256);
        c.submit();
        require(readBuffer(readback.Get(), 64) == values,
                "Allocator reset contains stale commands");
    }
    std::cout << "Verified 4 allocator/list generations after completed fence values\n";
}

int runChapter(int argc, char** argv) {
    auto options = labOptions(argc, argv, 44);
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
