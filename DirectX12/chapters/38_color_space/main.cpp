// sRGB、线性光照与显示编码
// 本目录拥有完整课程源码；只共享第三方依赖，不链接其他章节的课程实现。
// 本章检查：albedo；在线性空间光照。
// 本章检查：数据贴图；normal/MR/AO 不做颜色 gamma。
// 本章检查：mip 颜色；alpha 不跟 RGB 做同一 gamma。
// 读代码顺序：资源所有权 → 管线/绑定 → 录制和提交 → 完成检查 → 逆序释放。
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

void writeBuffer(ID3D12Resource* resource, const void* data, size_t bytes) {
    void* p{};
    D3D12_RANGE empty{0, 0};
    dxCheck(resource->Map(0, &empty, &p));
    std::memcpy(p, data, bytes);
    resource->Unmap(0, nullptr);
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
            // 计算 PSO 与根签名配套；后续 Dispatch 使用这里选择的入口。
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
                // 状态迁移描述资源下一步的真实用途；不能用 CPU 等待替代所需的状态。
                c.cmd->ResourceBarrier(2, barriers);
                auto gpu = heap->GetGPUDescriptorHandleForHeapStart();
                gpu.ptr += UINT64((level - 1) * 2) * stride;
                c.cmd->SetComputeRootDescriptorTable(0, gpu);
                gpu.ptr += stride;
                c.cmd->SetComputeRootDescriptorTable(1, gpu);
                // Dispatch 的单位是工作组，线程组尺寸由 HLSL numthreads 声明。
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

struct RenderState {
    std::unique_ptr<GpuTexture> texture;
};

int graphicsLab(int argc, char** argv, const LabOptions& options) {
    int mode = ((6));
    Geometry mesh;
    DxRenderOptions render;
    render.captureStem = "dx_lab_" + std::to_string(35);
    render.redOnly = false;
    render.cull = (D3D12_CULL_MODE_NONE);
    render.clampSampler = false;
    if (!mesh.vertices.empty())
        render.mesh = &mesh;
    render.setup = [=](DxScene& scene) {
        auto state = std::make_shared<RenderState>();
        scene.extension = state;
        {
            auto pixels = imageFile(options.directory / "assets/textures/albedo.png");
            state->texture =
                std::make_unique<GpuTexture>(scene.c, options, std::move(pixels), false, true);
            state->texture->attach(scene, false);
        }
    };
    // 计算先独立提交完成，再开始图形命令录制，避免在同一 allocator 正录制时把它重置。
    render.beforeFrame = [](DxScene& scene, float time) {
        // The compute submission must finish before DxWindow resets its allocator to record graphics.
    };
    return dxMain(argc, argv, mode, render);
}

int runChapter(int argc, char** argv) {
    auto options = labOptions(argc, argv, 35);
    {
        return graphicsLab(argc, argv, options);
    }
}
} // namespace course

int main(int argc, char** argv) try {
    return course::runChapter(argc, argv);
} catch (const std::exception& error) {
    std::cerr << "ERROR: " << error.what() << '\n';
    return 1;
}
