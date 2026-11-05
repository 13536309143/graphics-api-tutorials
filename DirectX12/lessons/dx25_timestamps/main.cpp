// 关联教材：GPU 时间戳、管线缓存与测量边界。
// 关键约束：计时单位；分别解释 GPU 与 CPU 耗时。
// 关键约束：缓存输入；不是所有版本/驱动可通用数据。
// 关键约束：结论范围；不是跨 API benchmark。
// dx25_timestamps
// 本目录拥有完整课程源码；只共享第三方依赖，不链接其他章节的课程实现。
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

void timestamps(DxDevice& c) {
    D3D12_QUERY_HEAP_DESC desc{};
    desc.Type = D3D12_QUERY_HEAP_TYPE_TIMESTAMP;
    desc.Count = 2;
    ComPtr<ID3D12QueryHeap> heap;
    dxCheck(c.device->CreateQueryHeap(&desc, IID_PPV_ARGS(&heap)));
    auto upload = c.buffer(1024 * 1024, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ);
    auto gpu = c.buffer(1024 * 1024, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_COPY_DEST);
    auto result = c.buffer(16, D3D12_HEAP_TYPE_READBACK, D3D12_RESOURCE_STATE_COPY_DEST);
    std::vector<uint8_t> bytes(1024 * 1024);
    writeBuffer(upload.Get(), bytes.data(), bytes.size());
    c.begin();
    c.cmd->EndQuery(heap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, 0);
    c.cmd->CopyBufferRegion(gpu.Get(), 0, upload.Get(), 0, bytes.size());
    c.cmd->EndQuery(heap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, 1);
    c.cmd->ResolveQueryData(heap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, 0, 2, result.Get(), 0);
    c.submit();
    auto words = readBuffer(result.Get(), 4);
    uint64_t ticks[2]{};
    std::memcpy(ticks, words.data(), 16);
    UINT64 frequency{};
    dxCheck(c.queue->GetTimestampFrequency(&frequency));
    require(frequency != 0, "Invalid timestamp frequency");
    std::cout << "GPU copy measurement=" << double(ticks[1] - ticks[0]) * 1000 / frequency
              << " ms (not a cross-API benchmark)\n";
}

int runChapter(int argc, char** argv) {
    auto options = labOptions(argc, argv, 25);
    DxDevice c(options.warp);
    {
        timestamps(c);
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
