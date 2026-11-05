// 关联教材：常量结构、矩阵存储与对齐。
// 关键约束：二进制布局；settings 偏移=64。
// 关键约束：矩阵约定；存储与乘法记法分别核对。
// 关键约束：常量偏移；alignas(16) 不覆盖全部接口规则。
// dx51_uniform_layout
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

// 查询、诊断与字节检查有明确的结果；合法 INFO 消息可能被默认过滤，临时空过滤器只扩大接收范围。
void inspection(DxDevice& c, const LabOptions& options) {
    static_assert(sizeof(Frame) == 64 && sizeof(Draw) == 80 && offsetof(Draw, settings) == 64);
    ComputeKernel k(c, options);
    k.execute(1, 128, 0, 0, 2);
    auto r = readBuffer(k.readback.Get(), 128);
    for (uint32_t i = 0; i < 128; ++i)
        require(r[i] == i * 7 + 13, "CBV vector layout mismatch");
    std::cout << "Verified Frame=64, Draw=80, offset=64 and 256-byte-aligned CBV selection\n";
}

int runChapter(int argc, char** argv) {
    auto options = labOptions(argc, argv, 51);
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
