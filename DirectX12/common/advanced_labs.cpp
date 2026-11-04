#define STB_IMAGE_IMPLEMENTATION
#include "advanced.hpp"
#include "advanced_labs.hpp"
#include "dx12_course.hpp"
#include <numeric>
#include <d3d11shader.h>

namespace course {
namespace {
ComPtr<ID3DBlob>
labShader(const LabOptions& options, const char* entry, const char* profile = "cs_5_1") {
    ComPtr<ID3DBlob> result, error;
    UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifndef NDEBUG
    flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
    auto path = options.directory / "shaders/lab.hlsl";
    auto hr = D3DCompileFromFile(path.c_str(),
                                 nullptr,
                                 D3D_COMPILE_STANDARD_FILE_INCLUDE,
                                 entry,
                                 profile,
                                 flags,
                                 0,
                                 &result,
                                 &error);
    if (error)
        std::cerr << static_cast<const char*>(error->GetBufferPointer());
    dxCheck(hr, "Lab shader compilation");
    return result;
}
// fence 单调完成值保护复用与回收，不采用 Vulkan binary fence 的 reset 模型。
void waitValue(DxDevice& c, UINT64 value) {
    if (c.fence->GetCompletedValue() < value) {
        dxCheck(c.fence->SetEventOnCompletion(value, c.event));
        require(WaitForSingleObject(c.event, 30000) == WAIT_OBJECT_0, "Fence timeout");
    }
}
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
// 每个计算槽拥有 DEFAULT/UAV、READBACK 和 256 字节对齐常量；状态与引用分别维护。
struct ComputeKernel {
    DxDevice& c;
    ComPtr<ID3D12Resource> output, readback, parameters;
    ComPtr<ID3D12RootSignature> signature;
    ComPtr<ID3D12PipelineState> pipeline;
    ComPtr<ID3DBlob> compiledShader;
    D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    ComputeKernel(DxDevice& context,
                  const LabOptions& options,
                  ID3DBlob* cache = nullptr,
                  ID3DBlob* exactShader = nullptr)
        : c(context) {
        output = c.buffer(4096,
                          D3D12_HEAP_TYPE_DEFAULT,
                          D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                          D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
        readback = c.buffer(4096, D3D12_HEAP_TYPE_READBACK, D3D12_RESOURCE_STATE_COPY_DEST);
        parameters = c.buffer(3 * 256, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ);
        std::vector<uint8_t> bytes(3 * 256);
        uint32_t values[3][4] = {{2, 3, 0, 0}, {5, 11, 0, 0}, {7, 13, 0, 0}};
        for (uint32_t i = 0; i < 3; ++i)
            std::memcpy(bytes.data() + i * 256, values[i], 16);
        writeBuffer(parameters.Get(), bytes.data(), bytes.size());
        D3D12_ROOT_PARAMETER params[3]{};
        params[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
        params[0].Constants = {0, 0, 4};
        params[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV;
        params[1].Descriptor.ShaderRegister = 0;
        params[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        params[2].Descriptor.ShaderRegister = 1;
        D3D12_ROOT_SIGNATURE_DESC desc{};
        desc.NumParameters = 3;
        desc.pParameters = params;
        signature = rootSignature(c, desc);
        compiledShader =
            exactShader ? ComPtr<ID3DBlob>(exactShader) : labShader(options, "ComputeMain");
        auto shader = compiledShader;
        D3D12_COMPUTE_PIPELINE_STATE_DESC info{};
        info.pRootSignature = signature.Get();
        info.CS = {shader->GetBufferPointer(), shader->GetBufferSize()};
        if (cache)
            info.CachedPSO = {cache->GetBufferPointer(), cache->GetBufferSize()};
        dxCheck(c.device->CreateComputePipelineState(&info, IID_PPV_ARGS(&pipeline)));
    }
    void record(ID3D12GraphicsCommandList* cmd,
                uint32_t mode,
                uint32_t count,
                uint32_t factor,
                uint32_t bias,
                uint32_t slot = 0) {
        if (state != D3D12_RESOURCE_STATE_UNORDERED_ACCESS) {
            auto b = transition(output.Get(), state, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            cmd->ResourceBarrier(1, &b);
        }
        uint32_t constants[4] = {mode, count, factor, bias};
        cmd->SetPipelineState(pipeline.Get());
        cmd->SetComputeRootSignature(signature.Get());
        cmd->SetComputeRoot32BitConstants(0, 4, constants, 0);
        cmd->SetComputeRootUnorderedAccessView(1, output->GetGPUVirtualAddress());
        cmd->SetComputeRootConstantBufferView(2, parameters->GetGPUVirtualAddress() + slot * 256);
        cmd->Dispatch((count + 63) / 64, 1, 1);
        auto barrier = transition(
            output.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);
        cmd->ResourceBarrier(1, &barrier);
        cmd->CopyBufferRegion(readback.Get(), 0, output.Get(), 0, 4096);
        state = D3D12_RESOURCE_STATE_COPY_SOURCE;
    }
    void execute(uint32_t mode, uint32_t count, uint32_t factor, uint32_t bias, uint32_t slot = 0) {
        c.begin();
        record(c.cmd.Get(), mode, count, factor, bias, slot);
        c.submit();
    }
};
void capabilities(DxDevice& c) {
    D3D12_FEATURE_DATA_D3D12_OPTIONS options{};
    dxCheck(c.device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &options, sizeof(options)));
    D3D12_FEATURE_DATA_FORMAT_SUPPORT format{DXGI_FORMAT_R8G8B8A8_UNORM};
    dxCheck(c.device->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &format, sizeof(format)));
    require((format.Support1 & D3D12_FORMAT_SUPPORT1_RENDER_TARGET) != 0,
            "RGBA8 render target unsupported");
    std::cout << "ResourceBindingTier=" << options.ResourceBindingTier
              << ", heapTier=" << options.ResourceHeapTier
              << ", CBV alignment=" << D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT
              << ", texture pitch alignment=" << D3D12_TEXTURE_DATA_PITCH_ALIGNMENT << '\n';
    D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS ms{};
    ms.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    ms.SampleCount = 4;
    dxCheck(
        c.device->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &ms, sizeof(ms)));
    std::cout << "RGBA8 4x MSAA quality levels=" << ms.NumQualityLevels << '\n';
}
// UPLOAD 写入后复制到 DEFAULT，再转 COPY_SOURCE 并读回；upload 保留到 fence 完成。
void stagingCopy(DxDevice& c) {
    std::vector<uint32_t> original(257);
    for (uint32_t i = 0; i < 257; ++i)
        original[i] = i * 17 + 9;
    auto upload = c.buffer(257 * 4, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ);
    auto gpu = c.buffer(257 * 4, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_COPY_DEST);
    auto readback = c.buffer(257 * 4, D3D12_HEAP_TYPE_READBACK, D3D12_RESOURCE_STATE_COPY_DEST);
    writeBuffer(upload.Get(), original.data(), original.size() * 4);
    c.begin();
    c.cmd->CopyBufferRegion(gpu.Get(), 0, upload.Get(), 0, 257 * 4);
    auto b =
        transition(gpu.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COPY_SOURCE);
    c.cmd->ResourceBarrier(1, &b);
    c.cmd->CopyBufferRegion(readback.Get(), 0, gpu.Get(), 0, 257 * 4);
    c.submit();
    require(readBuffer(readback.Get(), 257) == original, "DEFAULT staging-copy result mismatch");
    std::cout << "Verified 257 DEFAULT round-trip words\n";
}
// 两组 allocator/list 与可变资源交替提交；等待的是本槽上次使用，不是每次提交后立即全局等待。
void frameRing(DxDevice& c, const LabOptions& options) {
    ComputeKernel kernels[2] = {ComputeKernel(c, options), ComputeKernel(c, options)};
    ComPtr<ID3D12CommandAllocator> allocators[2];
    ComPtr<ID3D12GraphicsCommandList> commands[2];
    UINT64 completed[2]{};
    int previous[2] = {-1, -1};
    for (uint32_t i = 0; i < 2; ++i) {
        dxCheck(c.device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
                                                 IID_PPV_ARGS(&allocators[i])));
        dxCheck(c.device->CreateCommandList(0,
                                            D3D12_COMMAND_LIST_TYPE_DIRECT,
                                            allocators[i].Get(),
                                            nullptr,
                                            IID_PPV_ARGS(&commands[i])));
        dxCheck(commands[i]->Close());
    }
    for (uint32_t frame = 0; frame < 8; ++frame) {
        uint32_t slot = frame % 2;
        waitValue(c, completed[slot]);
        if (previous[slot] >= 0) {
            auto result = readBuffer(kernels[slot].readback.Get(), 257);
            for (uint32_t i = 0; i < 257; ++i)
                require(result[i] == i * 3 + uint32_t(previous[slot]),
                        "In-flight slot overwritten");
        }
        dxCheck(allocators[slot]->Reset());
        dxCheck(commands[slot]->Reset(allocators[slot].Get(), nullptr));
        kernels[slot].record(commands[slot].Get(), 0, 257, 3, frame);
        dxCheck(commands[slot]->Close());
        ID3D12CommandList* lists[] = {commands[slot].Get()};
        c.queue->ExecuteCommandLists(1, lists);
        completed[slot] = ++c.value;
        dxCheck(c.queue->Signal(c.fence.Get(), completed[slot]));
        previous[slot] = int(frame);
    }
    for (uint32_t slot = 0; slot < 2; ++slot) {
        waitValue(c, completed[slot]);
        auto result = readBuffer(kernels[slot].readback.Get(), 257);
        for (uint32_t i = 0; i < 257; ++i)
            require(result[i] == i * 3 + uint32_t(previous[slot]), "Final slot mismatch");
    }
    std::cout << "Verified 8 submissions using 2 independent allocator/buffer slots\n";
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
// 线性 UNORM mip 在 GPU 上由 SRV→UAV 逐级生成；sRGB 材质链用线性平均后再编码的 CPU 路径。
struct GpuTexture {
    DxDevice& c;
    ComPtr<ID3D12Resource> texture;
    std::vector<Pixels> reference;
    DXGI_FORMAT viewFormat;
    GpuTexture(
        DxDevice& context, const LabOptions& options, Pixels pixels, bool mips, bool srgb = false)
        : c(context) {
        reference = mips ? mipReference(pixels, srgb) : std::vector<Pixels>{pixels};
        const UINT levels = UINT(reference.size());
        const bool gpuMips = mips && !srgb;
        viewFormat = srgb ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM;
        D3D12_RESOURCE_DESC desc{};
        desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        desc.Width = pixels.width;
        desc.Height = pixels.height;
        desc.DepthOrArraySize = 1;
        desc.MipLevels = UINT16(levels);
        desc.Format = srgb ? DXGI_FORMAT_R8G8B8A8_TYPELESS : DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc.Count = 1;
        if (gpuMips)
            desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        D3D12_HEAP_PROPERTIES hp{};
        hp.Type = D3D12_HEAP_TYPE_DEFAULT;
        dxCheck(c.device->CreateCommittedResource(&hp,
                                                  D3D12_HEAP_FLAG_NONE,
                                                  &desc,
                                                  D3D12_RESOURCE_STATE_COPY_DEST,
                                                  nullptr,
                                                  IID_PPV_ARGS(&texture)));
        std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> footprints(levels);
        std::vector<UINT> rows(levels);
        std::vector<UINT64> rowBytes(levels);
        UINT64 total{};
        c.device->GetCopyableFootprints(
            &desc, 0, levels, 0, footprints.data(), rows.data(), rowBytes.data(), &total);
        auto upload = c.buffer(total, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ);
        std::vector<uint8_t> packed(static_cast<size_t>(total));
        UINT copied = gpuMips ? 1 : levels;
        for (UINT level = 0; level < copied; ++level)
            for (UINT y = 0; y < reference[level].height; ++y)
                std::memcpy(packed.data() + footprints[level].Offset +
                                size_t(y) * footprints[level].Footprint.RowPitch,
                            reference[level].rgba.data() + size_t(y) * reference[level].width * 4,
                            reference[level].width * 4);
        writeBuffer(upload.Get(), packed.data(), packed.size());
        c.begin();
        for (UINT level = 0; level < copied; ++level) {
            D3D12_TEXTURE_COPY_LOCATION src{}, dst{};
            src.pResource = upload.Get();
            src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
            src.PlacedFootprint = footprints[level];
            dst.pResource = texture.Get();
            dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
            dst.SubresourceIndex = level;
            c.cmd->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);
        }
        ComPtr<ID3D12DescriptorHeap> heap;
        ComPtr<ID3D12RootSignature> signature;
        ComPtr<ID3D12PipelineState> pipeline;
        // 每级 SRV/UAV 指向不同 mip，状态转换覆盖真正的子资源；临时 heap 与管线活到提交完成。
        if (gpuMips) {
            D3D12_FEATURE_DATA_FORMAT_SUPPORT support{DXGI_FORMAT_R8G8B8A8_UNORM};
            dxCheck(c.device->CheckFeatureSupport(
                D3D12_FEATURE_FORMAT_SUPPORT, &support, sizeof(support)));
            require((support.Support2 & D3D12_FORMAT_SUPPORT2_UAV_TYPED_STORE) != 0,
                    "RGBA8 typed UAV stores unsupported");
            D3D12_DESCRIPTOR_HEAP_DESC h{};
            h.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
            h.NumDescriptors = (levels - 1) * 2;
            h.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
            dxCheck(c.device->CreateDescriptorHeap(&h, IID_PPV_ARGS(&heap)));
            D3D12_DESCRIPTOR_RANGE ranges[2]{};
            D3D12_ROOT_PARAMETER parameters[2]{};
            for (UINT i = 0; i < 2; ++i) {
                ranges[i].RangeType =
                    i == 0 ? D3D12_DESCRIPTOR_RANGE_TYPE_SRV : D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
                ranges[i].NumDescriptors = 1;
                ranges[i].BaseShaderRegister = i == 0 ? 0 : 2;
                ranges[i].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
                parameters[i].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
                parameters[i].DescriptorTable = {1, &ranges[i]};
            }
            D3D12_ROOT_SIGNATURE_DESC r{};
            r.NumParameters = 2;
            r.pParameters = parameters;
            signature = rootSignature(c, r);
            auto shader = labShader(options, "MipMain");
            D3D12_COMPUTE_PIPELINE_STATE_DESC p{};
            p.pRootSignature = signature.Get();
            p.CS = {shader->GetBufferPointer(), shader->GetBufferSize()};
            dxCheck(c.device->CreateComputePipelineState(&p, IID_PPV_ARGS(&pipeline)));
            UINT stride = c.device->GetDescriptorHandleIncrementSize(h.Type);
            auto cpu = heap->GetCPUDescriptorHandleForHeapStart();
            for (UINT level = 1; level < levels; ++level) {
                D3D12_SHADER_RESOURCE_VIEW_DESC srv{};
                srv.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
                srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
                srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
                srv.Texture2D.MostDetailedMip = level - 1;
                srv.Texture2D.MipLevels = 1;
                auto handle = cpu;
                handle.ptr += SIZE_T((level - 1) * 2) * stride;
                c.device->CreateShaderResourceView(texture.Get(), &srv, handle);
                D3D12_UNORDERED_ACCESS_VIEW_DESC uav{};
                uav.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
                uav.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
                uav.Texture2D.MipSlice = level;
                handle.ptr += stride;
                c.device->CreateUnorderedAccessView(texture.Get(), nullptr, &uav, handle);
            }
            ID3D12DescriptorHeap* heaps[] = {heap.Get()};
            c.cmd->SetDescriptorHeaps(1, heaps);
            c.cmd->SetComputeRootSignature(signature.Get());
            c.cmd->SetPipelineState(pipeline.Get());
            for (UINT level = 1; level < levels; ++level) {
                auto from = level == 1 ? D3D12_RESOURCE_STATE_COPY_DEST
                                       : D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
                auto src =
                    transition(texture.Get(), from, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
                src.Transition.Subresource = level - 1;
                auto dst = transition(texture.Get(),
                                      D3D12_RESOURCE_STATE_COPY_DEST,
                                      D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
                dst.Transition.Subresource = level;
                D3D12_RESOURCE_BARRIER barriers[] = {src, dst};
                c.cmd->ResourceBarrier(2, barriers);
                auto gpu = heap->GetGPUDescriptorHandleForHeapStart();
                gpu.ptr += UINT64((level - 1) * 2) * stride;
                c.cmd->SetComputeRootDescriptorTable(0, gpu);
                gpu.ptr += stride;
                c.cmd->SetComputeRootDescriptorTable(1, gpu);
                c.cmd->Dispatch(
                    (reference[level].width + 7) / 8, (reference[level].height + 7) / 8, 1);
            }
            for (UINT level = 0; level < levels; ++level) {
                auto from = level + 1 == levels ? D3D12_RESOURCE_STATE_UNORDERED_ACCESS
                                                : D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
                auto b =
                    transition(texture.Get(), from, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
                b.Transition.Subresource = level;
                c.cmd->ResourceBarrier(1, &b);
            }
        } else {
            auto b = transition(texture.Get(),
                                D3D12_RESOURCE_STATE_COPY_DEST,
                                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
            c.cmd->ResourceBarrier(1, &b);
        }
        c.submit(); // Local descriptors, pipeline and upload storage may be released only now.
        std::cout << "Uploaded " << pixels.width << 'x' << pixels.height << ", mipLevels=" << levels
                  << ", generation=" << (gpuMips ? "GPU compute" : "CPU/upload")
                  << ", sRGB=" << srgb << '\n';
    }
    void attach(DxScene& scene, bool lastMip = false) {
        D3D12_SHADER_RESOURCE_VIEW_DESC srv{};
        srv.Format = viewFormat;
        srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srv.Texture2D.MostDetailedMip = lastMip ? UINT(reference.size() - 1) : 0;
        srv.Texture2D.MipLevels = lastMip ? 1 : UINT(reference.size());
        c.device->CreateShaderResourceView(
            texture.Get(), &srv, scene.srvHeap->GetCPUDescriptorHandleForHeapStart());
    }
    void verify() {
        auto desc = texture->GetDesc();
        UINT levels = UINT(reference.size());
        std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> footprints(levels);
        UINT64 total{};
        c.device->GetCopyableFootprints(
            &desc, 0, levels, 0, footprints.data(), nullptr, nullptr, &total);
        auto readback = c.buffer(total, D3D12_HEAP_TYPE_READBACK, D3D12_RESOURCE_STATE_COPY_DEST);
        c.begin();
        auto b = transition(texture.Get(),
                            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                            D3D12_RESOURCE_STATE_COPY_SOURCE);
        c.cmd->ResourceBarrier(1, &b);
        for (UINT level = 0; level < levels; ++level) {
            D3D12_TEXTURE_COPY_LOCATION src{}, dst{};
            src.pResource = texture.Get();
            src.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
            src.SubresourceIndex = level;
            dst.pResource = readback.Get();
            dst.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
            dst.PlacedFootprint = footprints[level];
            c.cmd->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);
        }
        b = transition(texture.Get(),
                       D3D12_RESOURCE_STATE_COPY_SOURCE,
                       D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        c.cmd->ResourceBarrier(1, &b);
        c.submit();
        void* p{};
        D3D12_RANGE range{0, SIZE_T(total)};
        dxCheck(readback->Map(0, &range, &p));
        for (UINT level = 0; level < levels; ++level)
            for (UINT y = 0; y < reference[level].height; ++y) {
                auto* row = static_cast<uint8_t*>(p) + footprints[level].Offset +
                            size_t(y) * footprints[level].Footprint.RowPitch;
                for (UINT x = 0; x < reference[level].width * 4; ++x)
                    require(
                        std::abs(
                            int(row[x]) -
                            reference[level].rgba[size_t(y) * reference[level].width * 4 + x]) <= 3,
                        "Mip texel verification failed");
            }
        D3D12_RANGE empty{0, 0};
        readback->Unmap(0, &empty);
        std::cout << "Verified every texel of " << levels << " mip levels\n";
    }
};
// 计算结果资源从 UAV 转为顶点用途；VBV 直接引用 GPU 缓冲，CPU 不重建粒子位置。
struct ParticleKernel {
    DxDevice& c;
    ComPtr<ID3D12Resource> vertices;
    ComPtr<ID3D12RootSignature> signature;
    ComPtr<ID3D12PipelineState> pipeline;
    D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    ParticleKernel(DxDevice& context, const LabOptions& options) : c(context) {
        vertices = c.buffer(96 * 3 * sizeof(Vertex),
                            D3D12_HEAP_TYPE_DEFAULT,
                            D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                            D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
        D3D12_ROOT_PARAMETER params[2]{};
        params[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV;
        params[0].Descriptor.ShaderRegister = 1;
        params[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
        params[1].Constants = {2, 0, 2};
        D3D12_ROOT_SIGNATURE_DESC desc{};
        desc.NumParameters = 2;
        desc.pParameters = params;
        signature = rootSignature(c, desc);
        auto shader = labShader(options, "ParticleMain");
        D3D12_COMPUTE_PIPELINE_STATE_DESC info{};
        info.pRootSignature = signature.Get();
        info.CS = {shader->GetBufferPointer(), shader->GetBufferSize()};
        dxCheck(c.device->CreateComputePipelineState(&info, IID_PPV_ARGS(&pipeline)));
    }
    void execute(float time) {
        struct Tick {
            float time;
            uint32_t count;
        } tick{time, 96};
        c.begin();
        if (state != D3D12_RESOURCE_STATE_UNORDERED_ACCESS) {
            auto b = transition(vertices.Get(), state, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            c.cmd->ResourceBarrier(1, &b);
        }
        c.cmd->SetComputeRootSignature(signature.Get());
        c.cmd->SetPipelineState(pipeline.Get());
        c.cmd->SetComputeRootUnorderedAccessView(0, vertices->GetGPUVirtualAddress());
        c.cmd->SetComputeRoot32BitConstants(1, 2, &tick, 0);
        c.cmd->Dispatch(5, 1, 1);
        auto b = transition(vertices.Get(),
                            D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                            D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
        c.cmd->ResourceBarrier(1, &b);
        c.submit();
        state = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
    }
};
struct RenderState {
    std::unique_ptr<GpuTexture> texture;
    std::unique_ptr<ParticleKernel> particles;
};
int graphicsLab(int argc, char** argv, const LabOptions& options) {
    int id = options.id, mode = id == 27 ? 3 : id == 29 || id == 53 ? 4 : 6;
    Geometry mesh;
    if (id == 21)
        mesh = staticAsset(options.directory / "assets/models/vase.obj");
    if (id == 22 || id == 36)
        mesh = staticAsset(options.directory / "assets/models/robot.gltf");
    if (id == 29)
        mesh = particleGeometry();
    if (id == 52 || id == 53) {
        mesh = geometry(4);
        for (auto& vertex : mesh.vertices) {
            vertex.normal =
                id == 53 ? glm::vec3(1 - vertex.uv.x, vertex.uv.y, 1) : glm::vec3(0, 0, 1);
            vertex.uv *= 4.f;
        }
        if (id == 52)
            for (uint32_t i = 0; i < mesh.vertices.size(); ++i)
                mesh.indices.push_back(i);
    }
    DxRenderOptions render;
    render.captureStem = "dx_lab_" + std::to_string(id);
    render.redOnly = id == 43;
    render.cull = id == 50 ? D3D12_CULL_MODE_BACK : D3D12_CULL_MODE_NONE;
    render.clampSampler = id == 52;
    if (!mesh.vertices.empty())
        render.mesh = &mesh;
    if (id == 20 || id == 36)
        render.samples = 4;
    render.setup = [=](DxScene& scene) {
        auto state = std::make_shared<RenderState>();
        scene.extension = state;
        if (id == 17 || id == 19 || id == 35 || id == 36 || id == 41 || id == 52) {
            auto pixels = imageFile(options.directory / "assets/textures/albedo.png");
            state->texture = std::make_unique<GpuTexture>(scene.c,
                                                          options,
                                                          std::move(pixels),
                                                          id == 19 || id == 36 || id == 41,
                                                          id == 35 || id == 36);
            state->texture->attach(scene, id == 41);
        }
        if (id == 20 || id == 36) {
            for (auto format : {DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_D32_FLOAT}) {
                D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS q{};
                q.Format = format;
                q.SampleCount = 4;
                dxCheck(scene.c.device->CheckFeatureSupport(
                    D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &q, sizeof(q)));
                require(q.NumQualityLevels > 0, "4x MSAA format unsupported");
            }
        }
        if (id == 27) {
            D3D12_DRAW_ARGUMENTS arguments{3, 1, 0, 0};
            scene.indirectArguments = scene.c.buffer(
                sizeof(arguments), D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ);
            writeBuffer(scene.indirectArguments.Get(), &arguments, sizeof(arguments));
            D3D12_INDIRECT_ARGUMENT_DESC arg{};
            arg.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW;
            D3D12_COMMAND_SIGNATURE_DESC signature{};
            signature.ByteStride = sizeof(arguments);
            signature.NumArgumentDescs = 1;
            signature.pArgumentDescs = &arg;
            dxCheck(scene.c.device->CreateCommandSignature(
                &signature, nullptr, IID_PPV_ARGS(&scene.indirectSignature)));
        }
        if (id == 29) {
            state->particles = std::make_unique<ParticleKernel>(scene.c, options);
            scene.vb = {state->particles->vertices->GetGPUVirtualAddress(),
                        96 * 3 * sizeof(Vertex),
                        sizeof(Vertex)};
        }
        if (id == 34)
            scene.halfScissor = true;
        if (id == 49)
            scene.halfViewport = true;
        if (id == 23 || id == 36)
            scene.frameProvider = [](float aspect, float time, bool) {
                auto projection = glm::perspectiveRH_ZO(glm::radians(48.f), aspect, .1f, 100.f);
                glm::vec3 eye(std::sin(time * .3f) * 4, 1.6f, std::cos(time * .3f) * 4);
                return Frame{projection * glm::lookAtRH(eye, glm::vec3(0), glm::vec3(0, 1, 0))};
            };
    };
    // 计算先独立提交完成，再开始图形命令录制，避免在同一 allocator 正录制时把它重置。
    render.beforeFrame = [id](DxScene& scene, float time) {
        // The compute submission must finish before DxWindow resets its allocator to record graphics.
        if (id == 29)
            std::static_pointer_cast<RenderState>(scene.extension)->particles->execute(time);
    };
    return dxMain(argc, argv, mode, render);
}
void surfaceInspection(const LabOptions& options) {
    require(glfwInit() != 0, "GLFW init failed");
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    auto* window = glfwCreateWindow(960, 640, "DXGI inspection", nullptr, nullptr);
    require(window != nullptr, "Window failed");
    {
        DxWindow app(window, 2, options.warp);
        DXGI_SWAP_CHAIN_DESC1 desc{};
        dxCheck(app.swap->GetDesc1(&desc));
        require(desc.BufferCount == 2 && desc.SwapEffect == DXGI_SWAP_EFFECT_FLIP_DISCARD,
                "Unexpected swapchain configuration");
        app.render(1, nullptr);
        std::cout << "Verified HWND swapchain, buffers=" << desc.BufferCount
                  << ", index=" << app.swap->GetCurrentBackBufferIndex() << '\n';
    }
    glfwDestroyWindow(window);
    glfwTerminate();
}
// 查询、诊断与字节检查有明确的结果；合法 INFO 消息可能被默认过滤，临时空过滤器只扩大接收范围。
void inspection(DxDevice& c, const LabOptions& options, int id) {
    if (id == 37) {
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
    } else if (id == 38) {
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
    } else if (id == 39) {
        auto desc = c.queue->GetDesc();
        require(desc.Type == D3D12_COMMAND_LIST_TYPE_DIRECT, "Expected direct queue");
        std::cout << "queue type=" << desc.Type << ", priority=" << desc.Priority
                  << ", flags=" << desc.Flags << ", nodeMask=" << desc.NodeMask << '\n';
    } else if (id == 42) {
        auto shader = labShader(options, "ComputeMain");
        ComPtr<ID3D11ShaderReflection> reflection;
        dxCheck(D3DReflect(
            shader->GetBufferPointer(), shader->GetBufferSize(), IID_PPV_ARGS(&reflection)));
        UINT x{}, y{}, z{};
        reflection->GetThreadGroupSize(&x, &y, &z);
        require(x == 64 && y == 1 && z == 1, "Unexpected shader thread-group reflection");
        D3D11_SHADER_DESC desc{};
        dxCheck(reflection->GetDesc(&desc));
        std::cout << "Verified shader reflection: threads=" << x
                  << ", bindings=" << desc.BoundResources << '\n';
    } else if (id == 44) {
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
    } else if (id == 45) {
        ComputeKernel kernel(c, options);
        kernel.execute(0, 128, 2, 3);
        auto first = readBuffer(kernel.readback.Get(), 128);
        auto second = c.buffer(4096,
                               D3D12_HEAP_TYPE_DEFAULT,
                               D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                               D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
        auto readback = c.buffer(4096, D3D12_HEAP_TYPE_READBACK, D3D12_RESOURCE_STATE_COPY_DEST);
        c.begin();
        c.cmd->SetComputeRootSignature(kernel.signature.Get());
        c.cmd->SetPipelineState(kernel.pipeline.Get());
        uint32_t values[4] = {0, 128, 5, 11};
        c.cmd->SetComputeRoot32BitConstants(0, 4, values, 0);
        c.cmd->SetComputeRootUnorderedAccessView(1, second->GetGPUVirtualAddress());
        c.cmd->SetComputeRootConstantBufferView(2, kernel.parameters->GetGPUVirtualAddress());
        c.cmd->Dispatch(2, 1, 1);
        auto b = transition(
            second.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);
        c.cmd->ResourceBarrier(1, &b);
        c.cmd->CopyBufferRegion(readback.Get(), 0, second.Get(), 0, 4096);
        c.submit();
        auto result = readBuffer(readback.Get(), 128);
        for (uint32_t i = 0; i < 128; ++i)
            require(result[i] == i * 5 + 11, "Root UAV replacement mismatch");
        require(readBuffer(kernel.readback.Get(), 128) == first,
                "Prior output was unexpectedly changed");
        std::cout << "Verified root descriptor replacement after prior GPU use completed\n";
    } else if (id == 46) {
        for (auto format :
             {DXGI_FORMAT_D32_FLOAT, DXGI_FORMAT_D24_UNORM_S8_UINT, DXGI_FORMAT_D16_UNORM}) {
            D3D12_FEATURE_DATA_FORMAT_SUPPORT q{format};
            dxCheck(c.device->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &q, sizeof(q)));
            std::cout << "depth format=" << format << ", support1=" << q.Support1 << '\n';
        }
    } else if (id == 47) {
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
    } else if (id == 48) {
        auto initial = c.fence->GetCompletedValue();
        dxCheck(c.queue->Signal(c.fence.Get(), ++c.value));
        waitValue(c, c.value);
        require(c.fence->GetCompletedValue() > initial, "Fence counter failed to advance");
        dxCheck(c.queue->Signal(c.fence.Get(), ++c.value));
        waitValue(c, c.value);
        std::cout << "Verified monotonic fence reuse; DX12 fence is not reset like a binary Vulkan "
                     "fence\n";
    } else if (id == 51) {
        static_assert(sizeof(Frame) == 64 && sizeof(Draw) == 80 && offsetof(Draw, settings) == 64);
        ComputeKernel k(c, options);
        k.execute(1, 128, 0, 0, 2);
        auto r = readBuffer(k.readback.Get(), 128);
        for (uint32_t i = 0; i < 128; ++i)
            require(r[i] == i * 7 + 13, "CBV vector layout mismatch");
        std::cout << "Verified Frame=64, Draw=80, offset=64 and 256-byte-aligned CBV selection\n";
    } else if (id == 54) {
        capabilities(c);
        stagingCopy(c);
        GpuTexture mips(c, options, Pixels{64, 64, checker()}, true);
        mips.verify();
        frameRing(c, options);
        std::cout << "Completed buffer/texture/frame-slot acceptance suite\n";
    } else
        throw std::runtime_error("Unknown inspection id");
}

} // namespace
int dxLabMain(int argc, char** argv, int id) {
    auto options = labOptions(argc, argv, id);
    if (id == 17 || id == 19 || id == 20 || id == 21 || id == 22 || id == 23 || id == 27 ||
        id == 29 || id == 34 || id == 35 || id == 36 || id == 41 || id == 43 || id == 49 ||
        id == 50 || id == 52 || id == 53)
        return graphicsLab(argc, argv, options);
    if (id == 40) {
        surfaceInspection(options);
        return 0;
    }
    DxDevice c(options.warp);
    if (id >= 37)
        inspection(c, options, id);
    else if (id == 11)
        capabilities(c);
    else if (id == 12) {
        c.begin();
        dxCheck(c.cmd->Close());
        ID3D12CommandList* lists[] = {c.cmd.Get()};
        c.queue->ExecuteCommandLists(1, lists);
        c.wait();
        require(c.fence->GetCompletedValue() >= c.value, "Queue fence did not complete");
        std::cout << "Verified command list close/submit and fence completion\n";
    } else if (id == 13)
        stagingCopy(c);
    else if (id == 18) {
        GpuTexture texture(c, options, Pixels{64, 64, checker()}, true);
        texture.verify();
    } else if (id == 24)
        frameRing(c, options);
    else if (id == 25)
        timestamps(c);
    else if (id == 26) {
        dxCheck(c.queue->Signal(c.fence.Get(), ++c.value));
        dxCheck(c.queue->Wait(c.fence.Get(), c.value));
        dxCheck(c.queue->Signal(c.fence.Get(), ++c.value));
        waitValue(c, c.value);
        require(c.fence->GetCompletedValue() >= 2, "Fence counter did not advance");
        std::cout << "Verified queue producer 1 -> consumer 2 fence chain\n";
    } else if (id == 31) {
        for (int i = 0; i < 4; ++i)
            stagingCopy(c);
        std::cout << "Retired 4 UPLOAD/DEFAULT/READBACK generations after fence completion\n";
    } else if (id == 32) {
        ComPtr<ID3DBlob> cache, exactShader;
        {
            ComputeKernel first(c, options);
            first.execute(0, 257, 3, 7);
            require(readBuffer(first.readback.Get(), 257)[256] == 775,
                    "Initial cached PSO result mismatch");
            dxCheck(first.pipeline->GetCachedBlob(&cache));
            exactShader = first.compiledShader;
        }
        {
            ComputeKernel second(c, options, cache.Get(), exactShader.Get());
            second.execute(0, 257, 3, 7);
            require(readBuffer(second.readback.Get(), 257)[256] == 775,
                    "Reloaded cached PSO mismatch");
        }
        std::cout << "Verified cached PSO reload, bytes=" << cache->GetBufferSize() << '\n';
    } else if (id == 33) {
        dxCheck(c.queue->SetName(L"Course Graphics Queue"));
        dxCheck(c.cmd->SetName(L"Course Upload Commands"));
        dxCheck(c.allocator->SetName(L"Course Upload Allocator"));
        c.begin();
        c.submit();
        std::cout << "Named queue, command list and allocator for GPU debugging; event regions "
                     "require PIX helpers\n";
    } else {
        ComputeKernel kernel(c, options);
        uint32_t mode = id == 30 ? 2 : id == 14 || id == 15 ? 1 : 0;
        uint32_t count = id == 28 || id == 30 ? 257 : 128;
        for (uint32_t slot = 0; slot < (id == 15 ? 3u : 1u); ++slot) {
            uint32_t factor = id == 16 ? 9 : 3, bias = id == 16 ? 17 : 7;
            kernel.execute(mode, count, factor, bias, slot);
            auto result = readBuffer(kernel.readback.Get(), mode == 2 ? (count + 63) / 64 : count);
            if (mode == 2) {
                auto sum = std::accumulate(result.begin(), result.end(), 0u);
                require(sum == count * (count + 1) / 2, "Shared reduction result mismatch");
            } else {
                if (mode == 1) {
                    factor = slot == 0 ? 2 : slot == 1 ? 5 : 7;
                    bias = slot == 0 ? 3 : slot == 1 ? 11 : 13;
                }
                for (uint32_t i = 0; i < count; ++i)
                    require(result[i] == i * factor + bias, "Compute/root-binding mismatch");
            }
        }
        std::cout << "Verified experiment " << id << ", items=" << count
                  << ", constant stride=256\n";
    }
    c.checkMessages();
    return 0;
}
} // namespace course
