// 关联教材：综合场景、验收与工程化边界。
// 关键约束：可见综合；检查图像并说明场景范围。
// 关键约束：自动验收；数值与图像证据分别保留。
// 关键约束：工程下一步；先定义资源表与 pass 依赖再扩代码。
// dx54_acceptance_suite
// 本目录拥有完整课程源码；只共享第三方依赖，不链接其他章节的课程实现。
#define STB_IMAGE_IMPLEMENTATION
#include "support/advanced.hpp"
#include "support/dx12_course.hpp"
#include <numeric>
#include <d3d11shader.h>

namespace course {
ComPtr<ID3DBlob>
labShader(const LabOptions& options, const char* entry, const char* profile = "cs_5_1") {
    ComPtr<ID3DBlob> result, error;
    UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifndef NDEBUG
    flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
    const char* stage = std::strcmp(entry, "ParticleMain") == 0 ? "particles.hlsl"
                        : std::strcmp(entry, "MipMain") == 0    ? "mip.hlsl"
                                                                : "compute_lab.hlsl";
    auto path = options.directory / "shaders" / stage;
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
        // 计算 PSO 与根签名配套；后续 Dispatch 使用这里选择的入口。
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
            // 状态迁移描述资源下一步的真实用途；不能用 CPU 等待替代所需的状态。
            cmd->ResourceBarrier(1, &b);
        }
        uint32_t constants[4] = {mode, count, factor, bias};
        cmd->SetPipelineState(pipeline.Get());
        cmd->SetComputeRootSignature(signature.Get());
        cmd->SetComputeRoot32BitConstants(0, 4, constants, 0);
        cmd->SetComputeRootUnorderedAccessView(1, output->GetGPUVirtualAddress());
        cmd->SetComputeRootConstantBufferView(2, parameters->GetGPUVirtualAddress() + slot * 256);
        // Dispatch 的单位是工作组，线程组尺寸由 HLSL numthreads 声明。
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
        // 提交是异步的；Fence 完成值决定 allocator 与上传资源何时可复用。
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
        c.submit();
        // Local descriptors, pipeline and upload storage may be released only now.
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

// 查询、诊断与字节检查有明确的结果；合法 INFO 消息可能被默认过滤，临时空过滤器只扩大接收范围。
void inspection(DxDevice& c, const LabOptions& options) {
    capabilities(c);
    stagingCopy(c);
    GpuTexture mips(c, options, Pixels{64, 64, checker()}, true);
    mips.verify();
    frameRing(c, options);
    std::cout << "Completed buffer/texture/frame-slot acceptance suite\n";
}

int runChapter(int argc, char** argv) {
    auto options = labOptions(argc, argv, 54);
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
