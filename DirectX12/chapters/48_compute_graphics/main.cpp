// 计算生成顶点并直接绘制
// 本目录拥有完整课程源码；只共享第三方依赖，不链接其他章节的课程实现。
// 本章检查：GPU 输出布局；storage 与 vertex stride 一致。
// 本章检查：粒子规模；GPU 生成，CPU 不重传位置。
// 本章检查：消费依赖；当前等待路径不冒充 async compute。
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
        // 计算 PSO 与根签名配套；后续 Dispatch 使用这里选择的入口。
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
            // 状态迁移描述资源下一步的真实用途；不能用 CPU 等待替代所需的状态。
            c.cmd->ResourceBarrier(1, &b);
        }
        c.cmd->SetComputeRootSignature(signature.Get());
        c.cmd->SetPipelineState(pipeline.Get());
        c.cmd->SetComputeRootUnorderedAccessView(0, vertices->GetGPUVirtualAddress());
        c.cmd->SetComputeRoot32BitConstants(1, 2, &tick, 0);
        // Dispatch 的单位是工作组，线程组尺寸由 HLSL numthreads 声明。
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

    std::unique_ptr<ParticleKernel> particles;
};

int graphicsLab(int argc, char** argv, const LabOptions& options) {
    int mode = ((4));
    Geometry mesh;
    {
        mesh = particleGeometry();
    }
    DxRenderOptions render;
    render.captureStem = "dx_lab_" + std::to_string(29);
    render.redOnly = false;
    render.cull = (D3D12_CULL_MODE_NONE);
    render.clampSampler = false;
    if (!mesh.vertices.empty())
        render.mesh = &mesh;
    render.setup = [=](DxScene& scene) {
        auto state = std::make_shared<RenderState>();
        scene.extension = state;
        {
            state->particles = std::make_unique<ParticleKernel>(scene.c, options);
            scene.vb = {state->particles->vertices->GetGPUVirtualAddress(),
                        96 * 3 * sizeof(Vertex),
                        sizeof(Vertex)};
        }
    };
    // 计算先独立提交完成，再开始图形命令录制，避免在同一 allocator 正录制时把它重置。
    render.beforeFrame = [](DxScene& scene, float time) {
        // The compute submission must finish before DxWindow resets its allocator to record graphics.
        {
            std::static_pointer_cast<RenderState>(scene.extension)->particles->execute(time);
        }
    };
    return dxMain(argc, argv, mode, render);
}

int runChapter(int argc, char** argv) {
    auto options = labOptions(argc, argv, 29);
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
