#pragma once
#include <windows.h>
#include <wrl.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <d3d12sdklayers.h>
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include "scene.hpp"
#include <memory>
#include <iomanip>
#include <optional>
#include <functional>
namespace course {
using Microsoft::WRL::ComPtr;
inline std::filesystem::path dxShaderFile;
inline void dxCheck(HRESULT hr, const char* operation = "D3D12 call") {
    if (FAILED(hr)) {
        std::ostringstream s;
        s << operation << ": HRESULT 0x" << std::hex << unsigned(hr);
        throw std::runtime_error(s.str());
    }
}

// Debug 保留 HLSL 调试信息并关闭优化，便于在图形调试器里查看源码。
inline ComPtr<ID3DBlob> compileHlsl(const char* entry, const char* profile) {
    ComPtr<ID3DBlob> shader, error;
    UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifndef NDEBUG
    flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
    flags |= D3DCOMPILE_OPTIMIZATION_LEVEL3;
#endif
    auto result = D3DCompileFromFile(dxShaderFile.c_str(),
                                     nullptr,
                                     D3D_COMPILE_STANDARD_FILE_INCLUDE,
                                     entry,
                                     profile,
                                     flags,
                                     0,
                                     &shader,
                                     &error);
    if (error)
        std::cerr << static_cast<const char*>(error->GetBufferPointer());
    dxCheck(result, "HLSL compilation");
    return shader;
}

// 资源状态必须对应接下来的 GPU 用途，切换用途时显式提交 transition barrier。
inline D3D12_RESOURCE_BARRIER
transition(ID3D12Resource* resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after) {
    D3D12_RESOURCE_BARRIER b{};
    b.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    b.Transition = {resource, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, before, after};
    return b;
}

// 设备、命令队列、allocator/list 和 fence 构成最小提交链；ComPtr 管理 COM 引用。
struct DxDevice {
    ComPtr<IDXGIFactory6> factory;
    ComPtr<ID3D12Device> device;
    ComPtr<ID3D12CommandQueue> queue;
    ComPtr<ID3D12CommandAllocator> allocator;
    ComPtr<ID3D12GraphicsCommandList> cmd;
    ComPtr<ID3D12Fence> fence;
    ComPtr<ID3D12InfoQueue> messages;
    HANDLE event = nullptr;
    UINT64 value = 0;
    explicit DxDevice(bool warp = false) {
        UINT flags = 0;
        ComPtr<ID3D12Debug> debug;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug)))) {
            debug->EnableDebugLayer();
            flags |= DXGI_CREATE_FACTORY_DEBUG;
            std::cout << "D3D12 debug layer enabled\n";
        } else
            std::cout
                << "D3D12 debug layer unavailable; install Windows Graphics Tools for validation\n";
        dxCheck(CreateDXGIFactory2(flags, IID_PPV_ARGS(&factory)));
        ComPtr<IDXGIAdapter1> adapter;
        if (warp) {
            dxCheck(factory->EnumWarpAdapter(IID_PPV_ARGS(&adapter)));
            dxCheck(
                D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&device)));
        } else {
            for (UINT i = 0; factory->EnumAdapterByGpuPreference(
                                 i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&adapter)) !=
                             DXGI_ERROR_NOT_FOUND;
                 ++i) {
                DXGI_ADAPTER_DESC1 desc;
                adapter->GetDesc1(&desc);
                if (!(desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) &&
                    SUCCEEDED(D3D12CreateDevice(
                        adapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&device))))
                    break;
                adapter.Reset();
            }
            if (!device)
                throw std::runtime_error("No D3D12 feature level 12_0 adapter; try --warp");
        }
        DXGI_ADAPTER_DESC1 desc;
        adapter->GetDesc1(&desc);
        std::wcout << L"GPU: " << desc.Description << L'\n';
        device.As(&messages);
        D3D12_COMMAND_QUEUE_DESC qi{};
        qi.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        dxCheck(device->CreateCommandQueue(&qi, IID_PPV_ARGS(&queue)));
        dxCheck(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
                                               IID_PPV_ARGS(&allocator)));
        dxCheck(device->CreateCommandList(
            0, D3D12_COMMAND_LIST_TYPE_DIRECT, allocator.Get(), nullptr, IID_PPV_ARGS(&cmd)));
        dxCheck(cmd->Close());
        dxCheck(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));
        event = CreateEventW(nullptr, FALSE, FALSE, nullptr);
        if (!event)
            throw std::runtime_error("Cannot create fence event");
    }

    // allocator 只能在使用它的 GPU 命令完成后 Reset；本课程由 submit 中的等待保证。
    void begin() {
        dxCheck(allocator->Reset());
        dxCheck(cmd->Reset(allocator.Get(), nullptr));
    }
    void submit() {
        dxCheck(cmd->Close());
        ID3D12CommandList* lists[] = {cmd.Get()};
        queue->ExecuteCommandLists(1, lists);
        wait();
    }

    // 队列 signal fence 后，CPU 等事件确认进度；提交返回不等于 GPU 已完成。
    void wait() {
        dxCheck(queue->Signal(fence.Get(), ++value));
        if (fence->GetCompletedValue() < value) {
            dxCheck(fence->SetEventOnCompletion(value, event));
            if (WaitForSingleObject(event, 30000) != WAIT_OBJECT_0)
                throw std::runtime_error("GPU fence timeout");
        }
    }
    void checkMessages() {
        if (!messages)
            return;
        UINT64 count = messages->GetNumStoredMessagesAllowedByRetrievalFilter();
        for (UINT64 i = 0; i < count; ++i) {
            SIZE_T bytes = 0;
            messages->GetMessage(i, nullptr, &bytes);
            std::vector<uint8_t> storage(bytes);
            auto m = reinterpret_cast<D3D12_MESSAGE*>(storage.data());
            dxCheck(messages->GetMessage(i, m, &bytes));
            if (m->Severity == D3D12_MESSAGE_SEVERITY_ERROR ||
                m->Severity == D3D12_MESSAGE_SEVERITY_CORRUPTION)
                throw std::runtime_error(std::string("D3D12 validation: ") + m->pDescription);
        }
        messages->ClearStoredMessages();
    }
    ComPtr<ID3D12Resource> buffer(UINT64 size,
                                  D3D12_HEAP_TYPE heap,
                                  D3D12_RESOURCE_STATES state,
                                  D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE) {
        D3D12_HEAP_PROPERTIES hp{};
        hp.Type = heap;
        D3D12_RESOURCE_DESC rd{};
        rd.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        rd.Width = size;
        rd.Height = 1;
        rd.DepthOrArraySize = 1;
        rd.MipLevels = 1;
        rd.SampleDesc.Count = 1;
        rd.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        rd.Flags = flags;
        ComPtr<ID3D12Resource> result;
        dxCheck(device->CreateCommittedResource(
            &hp, D3D12_HEAP_FLAG_NONE, &rd, state, nullptr, IID_PPV_ARGS(&result)));
        return result;
    }
    ~DxDevice() {
        if (event)
            CloseHandle(event);
    }
};
inline ComPtr<ID3D12RootSignature> rootSignature(DxDevice& c,
                                                 const D3D12_ROOT_SIGNATURE_DESC& desc) {
    ComPtr<ID3DBlob> bytes, error;
    auto result = D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &bytes, &error);
    if (error)
        std::cerr << static_cast<const char*>(error->GetBufferPointer());
    dxCheck(result, "Root signature");
    ComPtr<ID3D12RootSignature> signature;
    dxCheck(c.device->CreateRootSignature(
        0, bytes->GetBufferPointer(), bytes->GetBufferSize(), IID_PPV_ARGS(&signature)));
    return signature;
}

// DEFAULT heap 用于 GPU 计算；CPU 通过 READBACK heap 读取拷贝出的结果。
inline void dxCompute(DxDevice& c) {
    auto storage = c.buffer(1024,
                            D3D12_HEAP_TYPE_DEFAULT,
                            D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                            D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS),
         readback = c.buffer(1024, D3D12_HEAP_TYPE_READBACK, D3D12_RESOURCE_STATE_COPY_DEST);
    D3D12_ROOT_PARAMETER root{};
    root.ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV;
    root.Descriptor.ShaderRegister = 0;
    D3D12_ROOT_SIGNATURE_DESC desc{};
    desc.NumParameters = 1;
    desc.pParameters = &root;
    auto signature = rootSignature(c, desc);
    auto cs = compileHlsl("ComputeMain", "cs_5_1");
    D3D12_COMPUTE_PIPELINE_STATE_DESC pso{};
    pso.pRootSignature = signature.Get();
    pso.CS = {cs->GetBufferPointer(), cs->GetBufferSize()};
    ComPtr<ID3D12PipelineState> pipeline;
    dxCheck(c.device->CreateComputePipelineState(&pso, IID_PPV_ARGS(&pipeline)));
    c.begin();
    c.cmd->SetComputeRootSignature(signature.Get());
    c.cmd->SetPipelineState(pipeline.Get());
    c.cmd->SetComputeRootUnorderedAccessView(0, storage->GetGPUVirtualAddress());
    c.cmd->Dispatch(4, 1, 1);
    auto b = transition(
        storage.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);
    c.cmd->ResourceBarrier(1, &b);
    c.cmd->CopyBufferRegion(readback.Get(), 0, storage.Get(), 0, 1024);
    c.submit();
    void* mapped;
    D3D12_RANGE readRange{0, 1024};
    dxCheck(readback->Map(0, &readRange, &mapped));
    for (uint32_t i = 0; i < 256; ++i)
        if (static_cast<uint32_t*>(mapped)[i] != i * i + 1)
            throw std::runtime_error("GPU compute mismatch");
    D3D12_RANGE written{0, 0};
    readback->Unmap(0, &written);
    std::cout << "Verified all 256 GPU compute results\n";
    c.checkMessages();
}
class DxScene {
public:
    DxDevice& c;
    int mode;
    Geometry mesh;
    ComPtr<ID3D12Resource> vertices, indices, uniform, texture;
    ComPtr<ID3D12DescriptorHeap> srvHeap;
    ComPtr<ID3D12RootSignature> signature;
    ComPtr<ID3D12PipelineState> pipeline;
    D3D12_VERTEX_BUFFER_VIEW vb{};
    D3D12_INDEX_BUFFER_VIEW ib{};
    std::optional<Frame> overrideFrame;
    std::function<Frame(float, float, bool)> frameProvider;
    std::vector<Draw> overrideDraws;
    bool halfScissor = false;
    bool halfViewport = false;
    ComPtr<ID3D12Resource> indirectArguments;
    ComPtr<ID3D12CommandSignature> indirectSignature;
    std::shared_ptr<void> extension;
    DxScene(DxDevice& context,
            int lesson,
            const Geometry* replacement = nullptr,
            UINT samples = 1,
            D3D12_CULL_MODE cull = D3D12_CULL_MODE_NONE,
            bool clampSampler = false,
            bool redOnly = false)
        : c(context), mode(lesson), mesh(replacement ? *replacement : geometry(mode)) {
        if (mode >= 4) {
            UINT size = UINT(mesh.vertices.size() * sizeof(Vertex));
            vertices = c.buffer(size, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ);
            void* mapped;
            D3D12_RANGE empty{0, 0};
            dxCheck(vertices->Map(0, &empty, &mapped));
            std::memcpy(mapped, mesh.vertices.data(), size);
            vertices->Unmap(0, nullptr);
            vb = {vertices->GetGPUVirtualAddress(), size, sizeof(Vertex)};
            if (!mesh.indices.empty()) {
                UINT bytes = UINT(mesh.indices.size() * 4);
                indices =
                    c.buffer(bytes, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ);
                dxCheck(indices->Map(0, &empty, &mapped));
                std::memcpy(mapped, mesh.indices.data(), bytes);
                indices->Unmap(0, nullptr);
                ib = {indices->GetGPUVirtualAddress(), bytes, DXGI_FORMAT_R32_UINT};
            }
        }

        // CBV GPU 地址要求 256 字节对齐，因此给 64 字节的帧矩阵预留 256 字节。
        uniform = c.buffer(256, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ);
        D3D12_HEAP_PROPERTIES hp{};
        hp.Type = D3D12_HEAP_TYPE_DEFAULT;
        D3D12_RESOURCE_DESC image{};
        image.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        image.Width = image.Height = 64;
        image.DepthOrArraySize = 1;
        image.MipLevels = 1;
        image.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        image.SampleDesc.Count = 1;
        dxCheck(c.device->CreateCommittedResource(&hp,
                                                  D3D12_HEAP_FLAG_NONE,
                                                  &image,
                                                  D3D12_RESOURCE_STATE_COPY_DEST,
                                                  nullptr,
                                                  IID_PPV_ARGS(&texture)));
        D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint;
        UINT rows;
        UINT64 rowBytes, total;

        // 纹理上传不能假定行数据紧密排列；按 footprint 的对齐行距逐行复制。
        c.device->GetCopyableFootprints(&image, 0, 1, 0, &footprint, &rows, &rowBytes, &total);
        auto upload = c.buffer(total, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ);
        void* mapped;
        D3D12_RANGE empty{0, 0};
        dxCheck(upload->Map(0, &empty, &mapped));
        auto data = checker();
        for (UINT y = 0; y < rows; ++y)
            std::memcpy(static_cast<uint8_t*>(mapped) + footprint.Offset +
                            size_t(y) * footprint.Footprint.RowPitch,
                        data.data() + size_t(y) * 64 * 4,
                        64 * 4);
        upload->Unmap(0, nullptr);
        c.begin();
        D3D12_TEXTURE_COPY_LOCATION source{}, destination{};
        source.pResource = upload.Get();
        source.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        source.PlacedFootprint = footprint;
        destination.pResource = texture.Get();
        destination.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        c.cmd->CopyTextureRegion(&destination, 0, 0, 0, &source, nullptr);
        auto b = transition(texture.Get(),
                            D3D12_RESOURCE_STATE_COPY_DEST,
                            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        c.cmd->ResourceBarrier(1, &b);
        c.submit();
        D3D12_DESCRIPTOR_HEAP_DESC heap{};
        heap.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        heap.NumDescriptors = 1;
        heap.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        dxCheck(c.device->CreateDescriptorHeap(&heap, IID_PPV_ARGS(&srvHeap)));
        D3D12_SHADER_RESOURCE_VIEW_DESC srv{};
        srv.Format = image.Format;
        srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srv.Texture2D.MipLevels = 1;
        c.device->CreateShaderResourceView(
            texture.Get(), &srv, srvHeap->GetCPUDescriptorHandleForHeapStart());

        // root signature 约定 b0 为帧矩阵、b1 为每绘制常量，t0 经 descriptor table 绑定纹理。
        D3D12_ROOT_PARAMETER params[3]{};
        params[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        params[0].Descriptor.ShaderRegister = 0;
        params[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
        params[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
        params[1].Constants = {1, 0, 20};
        params[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        D3D12_DESCRIPTOR_RANGE range{};
        range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        range.NumDescriptors = 1;
        range.BaseShaderRegister = 0;
        range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
        params[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        params[2].DescriptorTable = {1, &range};
        params[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
        D3D12_STATIC_SAMPLER_DESC sampler{};
        sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        sampler.AddressU = sampler.AddressV = sampler.AddressW =
            clampSampler ? D3D12_TEXTURE_ADDRESS_MODE_CLAMP : D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        sampler.MaxLOD = D3D12_FLOAT32_MAX;
        sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
        sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
        D3D12_ROOT_SIGNATURE_DESC root{};
        root.NumParameters = 3;
        root.pParameters = params;
        root.NumStaticSamplers = 1;
        root.pStaticSamplers = &sampler;
        root.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
        signature = rootSignature(c, root);
        auto vs = compileHlsl(mode == 3 ? "TriangleMain" : "VertexMain", "vs_5_1"),
             ps = compileHlsl("PixelMain", "ps_5_1");
        D3D12_INPUT_ELEMENT_DESC elements[] = {{"POSITION",
                                                0,
                                                DXGI_FORMAT_R32G32B32_FLOAT,
                                                0,
                                                0,
                                                D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
                                                0},
                                               {"NORMAL",
                                                0,
                                                DXGI_FORMAT_R32G32B32_FLOAT,
                                                0,
                                                12,
                                                D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
                                                0},
                                               {"TEXCOORD",
                                                0,
                                                DXGI_FORMAT_R32G32_FLOAT,
                                                0,
                                                24,
                                                D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
                                                0}};
        D3D12_GRAPHICS_PIPELINE_STATE_DESC pso{};
        pso.pRootSignature = signature.Get();
        pso.VS = {vs->GetBufferPointer(), vs->GetBufferSize()};
        pso.PS = {ps->GetBufferPointer(), ps->GetBufferSize()};
        pso.BlendState.RenderTarget[0].RenderTargetWriteMask =
            redOnly ? D3D12_COLOR_WRITE_ENABLE_RED | D3D12_COLOR_WRITE_ENABLE_ALPHA
                    : D3D12_COLOR_WRITE_ENABLE_ALL;
        pso.SampleMask = UINT_MAX;
        pso.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
        pso.RasterizerState.CullMode = cull;
        // 共用模型按外表面逆时针排列；明确指定正面，避免剔除后只剩内部。
        pso.RasterizerState.FrontCounterClockwise = TRUE;
        pso.RasterizerState.DepthClipEnable = TRUE;
        pso.DepthStencilState.DepthEnable = mode >= 5;
        pso.DepthStencilState.DepthWriteMask =
            mode >= 5 ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
        pso.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
        pso.DepthStencilState.StencilReadMask = pso.DepthStencilState.StencilWriteMask = 0xFF;
        if (mode >= 4)
            pso.InputLayout = {elements, 3};
        pso.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        pso.NumRenderTargets = 1;
        pso.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        pso.DSVFormat = DXGI_FORMAT_D32_FLOAT;
        pso.SampleDesc.Count = samples;
        dxCheck(c.device->CreateGraphicsPipelineState(&pso, IID_PPV_ARGS(&pipeline)));
    }

    // 绑定 PSO、root signature、descriptor heap 和顶点输入，再逐对象更新 root constants。
    void render(int w, int h, float time) {
        Frame f = overrideFrame.value_or(frame(mode, float(w) / h, false));
        if (frameProvider)
            f = frameProvider(float(w) / h, time, false);
        void* mapped;
        D3D12_RANGE empty{0, 0};
        dxCheck(uniform->Map(0, &empty, &mapped));
        std::memcpy(mapped, &f, sizeof(f));
        uniform->Unmap(0, nullptr);
        auto cmd = c.cmd.Get();
        cmd->SetPipelineState(pipeline.Get());
        cmd->SetGraphicsRootSignature(signature.Get());
        ID3D12DescriptorHeap* heaps[] = {srvHeap.Get()};
        cmd->SetDescriptorHeaps(1, heaps);
        cmd->SetGraphicsRootConstantBufferView(0, uniform->GetGPUVirtualAddress());
        cmd->SetGraphicsRootDescriptorTable(2, srvHeap->GetGPUDescriptorHandleForHeapStart());
        cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        if (mode >= 4) {
            cmd->IASetVertexBuffers(0, 1, &vb);
            if (indices)
                cmd->IASetIndexBuffer(&ib);
        }
        for (auto d : overrideDraws.empty() ? draws(mode, time) : overrideDraws) {
            cmd->SetGraphicsRoot32BitConstants(1, 20, &d, 0);
            if (indirectArguments)
                cmd->ExecuteIndirect(
                    indirectSignature.Get(), 1, indirectArguments.Get(), 0, nullptr, 0);
            else if (mode == 3)
                cmd->DrawInstanced(3, 1, 0, 0);
            else if (mode == 4)
                cmd->DrawInstanced(UINT(mesh.vertices.size()), 1, 0, 0);
            else
                cmd->DrawIndexedInstanced(UINT(mesh.indices.size()), mode == 7 ? 25 : 1, 0, 0, 0);
        }
    }
};
struct DxRenderOptions {
    const Geometry* mesh = nullptr;
    D3D12_CULL_MODE cull = D3D12_CULL_MODE_NONE;
    bool clampSampler = false, redOnly = false;
    std::string captureStem;
    UINT samples = 1;
    std::function<void(DxScene&)> setup;
    std::function<void(DxScene&, float)> update;
    std::function<void(DxScene&, float)> beforeFrame;
};
class DxWindow {
public:
    DxDevice c;
    GLFWwindow* window;
    int mode, width = 0, height = 0;
    ComPtr<IDXGISwapChain3> swap;
    ComPtr<ID3D12DescriptorHeap> rtvHeap, dsvHeap;
    ComPtr<ID3D12Resource> colors[2], depth, offscreen, multisample;
    DxRenderOptions options;
    std::unique_ptr<DxScene> scene;
    UINT rtvStride = 0;
    DxWindow(GLFWwindow* w, int lesson, bool warp, DxRenderOptions settings = {})
        : c(warp), window(w), mode(lesson), options(std::move(settings)) {
        D3D12_DESCRIPTOR_HEAP_DESC heap{};
        heap.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        heap.NumDescriptors = 4;
        dxCheck(c.device->CreateDescriptorHeap(&heap, IID_PPV_ARGS(&rtvHeap)));
        heap.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
        heap.NumDescriptors = 1;
        dxCheck(c.device->CreateDescriptorHeap(&heap, IID_PPV_ARGS(&dsvHeap)));
        rtvStride = c.device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        DXGI_SWAP_CHAIN_DESC1 desc{};
        desc.Width = 960;
        desc.Height = 640;
        desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        desc.BufferCount = 2;
        desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        ComPtr<IDXGISwapChain1> s;
        dxCheck(c.factory->CreateSwapChainForHwnd(
            c.queue.Get(), glfwGetWin32Window(w), &desc, nullptr, nullptr, &s));
        dxCheck(s.As(&swap));
        c.factory->MakeWindowAssociation(glfwGetWin32Window(w), DXGI_MWA_NO_ALT_ENTER);
        resize(960, 640);
        if (mode >= 3)
            scene = std::make_unique<DxScene>(c,
                                              mode,
                                              options.mesh,
                                              options.samples,
                                              options.cull,
                                              options.clampSampler,
                                              options.redOnly);
        if (scene && options.setup)
            options.setup(*scene);
    }
    D3D12_CPU_DESCRIPTOR_HANDLE rtv(UINT index) {
        auto h = rtvHeap->GetCPUDescriptorHandleForHeapStart();
        h.ptr += SIZE_T(index) * rtvStride;
        return h;
    }

    // ResizeBuffers 前必须等 GPU，并释放所有旧 back buffer 引用和相关附件。
    void resize(int w, int h) {
        if (width == w && height == h)
            return;
        c.wait();
        for (auto& image : colors)
            image.Reset();
        depth.Reset();
        offscreen.Reset();
        multisample.Reset();
        dxCheck(swap->ResizeBuffers(2, UINT(w), UINT(h), DXGI_FORMAT_R8G8B8A8_UNORM, 0));
        width = w;
        height = h;
        for (UINT i = 0; i < 2; ++i) {
            dxCheck(swap->GetBuffer(i, IID_PPV_ARGS(&colors[i])));
            c.device->CreateRenderTargetView(colors[i].Get(), nullptr, rtv(i));
        }
        D3D12_HEAP_PROPERTIES heap{};
        heap.Type = D3D12_HEAP_TYPE_DEFAULT;
        D3D12_RESOURCE_DESC desc{};
        desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        desc.Width = UINT64(w);
        desc.Height = UINT(h);
        desc.DepthOrArraySize = 1;
        desc.MipLevels = 1;
        desc.Format = DXGI_FORMAT_D32_FLOAT;
        desc.SampleDesc.Count = options.samples;
        desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
        D3D12_CLEAR_VALUE clear{};
        clear.Format = desc.Format;
        clear.DepthStencil.Depth = 1;
        dxCheck(c.device->CreateCommittedResource(&heap,
                                                  D3D12_HEAP_FLAG_NONE,
                                                  &desc,
                                                  D3D12_RESOURCE_STATE_DEPTH_WRITE,
                                                  &clear,
                                                  IID_PPV_ARGS(&depth)));
        c.device->CreateDepthStencilView(
            depth.Get(), nullptr, dsvHeap->GetCPUDescriptorHandleForHeapStart());
        if (options.samples > 1) {
            desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
            D3D12_CLEAR_VALUE msClear{};
            msClear.Format = desc.Format;
            dxCheck(c.device->CreateCommittedResource(&heap,
                                                      D3D12_HEAP_FLAG_NONE,
                                                      &desc,
                                                      D3D12_RESOURCE_STATE_RENDER_TARGET,
                                                      &msClear,
                                                      IID_PPV_ARGS(&multisample)));
            c.device->CreateRenderTargetView(multisample.Get(), nullptr, rtv(3));
        }
        if (mode == 10) {
            desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
            clear = {};
            clear.Format = desc.Format;
            dxCheck(c.device->CreateCommittedResource(&heap,
                                                      D3D12_HEAP_FLAG_NONE,
                                                      &desc,
                                                      D3D12_RESOURCE_STATE_RENDER_TARGET,
                                                      &clear,
                                                      IID_PPV_ARGS(&offscreen)));
            c.device->CreateRenderTargetView(offscreen.Get(), nullptr, rtv(2));
        }
    }
    void render(float time, const std::filesystem::path* capturePath) {

        // 每帧重新取得当前 back buffer；普通路径将其从 PRESENT 切到 RENDER_TARGET。
        auto back = colors[swap->GetCurrentBackBufferIndex()];
        c.begin();
        if (mode != 10) {
            auto b = transition(
                back.Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
            c.cmd->ResourceBarrier(1, &b);
        }
        auto color = rtv(options.samples > 1 ? 3
                         : mode == 10        ? 2
                                             : swap->GetCurrentBackBufferIndex()),
             ds = dsvHeap->GetCPUDescriptorHandleForHeapStart();
        float clear[] = {.035f, .055f, .09f, 1};
        c.cmd->ClearRenderTargetView(color, clear, 0, nullptr);
        c.cmd->ClearDepthStencilView(ds, D3D12_CLEAR_FLAG_DEPTH, 1, 0, 0, nullptr);
        c.cmd->OMSetRenderTargets(1, &color, FALSE, &ds);
        D3D12_VIEWPORT viewport{
            0, 0, float(width) / (scene && scene->halfViewport ? 2 : 1), float(height), 0, 1};
        D3D12_RECT scissor{0, 0, scene && scene->halfScissor ? width / 2 : width, height};
        c.cmd->RSSetViewports(1, &viewport);
        c.cmd->RSSetScissorRects(1, &scissor);
        if (scene) {
            if (options.update)
                options.update(*scene, time);
            scene->render(width, height, time);
        }

        // 离屏课先画独立纹理，再把它作为 COPY_SOURCE 拷贝到交换链图像。
        if (mode == 10) {
            D3D12_RESOURCE_BARRIER b[] = {transition(offscreen.Get(),
                                                     D3D12_RESOURCE_STATE_RENDER_TARGET,
                                                     D3D12_RESOURCE_STATE_COPY_SOURCE),
                                          transition(back.Get(),
                                                     D3D12_RESOURCE_STATE_PRESENT,
                                                     D3D12_RESOURCE_STATE_COPY_DEST)};
            c.cmd->ResourceBarrier(2, b);
            c.cmd->CopyResource(back.Get(), offscreen.Get());
            auto restore = transition(offscreen.Get(),
                                      D3D12_RESOURCE_STATE_COPY_SOURCE,
                                      D3D12_RESOURCE_STATE_RENDER_TARGET);
            c.cmd->ResourceBarrier(1, &restore);
        }
        auto old = mode == 10 ? D3D12_RESOURCE_STATE_COPY_DEST : D3D12_RESOURCE_STATE_RENDER_TARGET;
        if (options.samples > 1) {
            D3D12_RESOURCE_BARRIER barriers[] = {transition(multisample.Get(),
                                                            D3D12_RESOURCE_STATE_RENDER_TARGET,
                                                            D3D12_RESOURCE_STATE_RESOLVE_SOURCE),
                                                 transition(back.Get(),
                                                            D3D12_RESOURCE_STATE_RENDER_TARGET,
                                                            D3D12_RESOURCE_STATE_RESOLVE_DEST)};
            c.cmd->ResourceBarrier(2, barriers);
            c.cmd->ResolveSubresource(
                back.Get(), 0, multisample.Get(), 0, DXGI_FORMAT_R8G8B8A8_UNORM);
            auto restore = transition(multisample.Get(),
                                      D3D12_RESOURCE_STATE_RESOLVE_SOURCE,
                                      D3D12_RESOURCE_STATE_RENDER_TARGET);
            c.cmd->ResourceBarrier(1, &restore);
            old = D3D12_RESOURCE_STATE_RESOLVE_DEST;
        }
        ComPtr<ID3D12Resource> readback;
        D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint{};
        if (capturePath) {
            auto b = transition(back.Get(), old, D3D12_RESOURCE_STATE_COPY_SOURCE);
            c.cmd->ResourceBarrier(1, &b);
            auto desc = back->GetDesc();
            UINT rows;
            UINT64 bytes, total;
            c.device->GetCopyableFootprints(&desc, 0, 1, 0, &footprint, &rows, &bytes, &total);
            readback = c.buffer(total, D3D12_HEAP_TYPE_READBACK, D3D12_RESOURCE_STATE_COPY_DEST);
            D3D12_TEXTURE_COPY_LOCATION src{}, dst{};
            src.pResource = back.Get();
            src.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
            dst.pResource = readback.Get();
            dst.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
            dst.PlacedFootprint = footprint;
            c.cmd->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);
            old = D3D12_RESOURCE_STATE_COPY_SOURCE;
        }

        // 呈现前把 back buffer 恢复到 PRESENT；CPU 读回则在 GPU 拷贝及 fence 完成后进行。
        auto present = transition(back.Get(), old, D3D12_RESOURCE_STATE_PRESENT);
        c.cmd->ResourceBarrier(1, &present);
        c.submit();
        dxCheck(swap->Present(1, 0));
        c.wait();
        if (capturePath) {
            void* p;
            D3D12_RANGE range{0, SIZE_T(readback->GetDesc().Width)};
            dxCheck(readback->Map(0, &range, &p));
            std::vector<uint8_t> packed(size_t(width) * height * 4);
            for (int y = 0; y < height; ++y)
                std::memcpy(packed.data() + size_t(y) * width * 4,
                            static_cast<uint8_t*>(p) + footprint.Offset +
                                size_t(y) * footprint.Footprint.RowPitch,
                            size_t(width) * 4);
            D3D12_RANGE noWrite{0, 0};
            readback->Unmap(0, &noWrite);
            capture(*capturePath, packed.data(), width, height, false, mode >= 3);
        }
        c.checkMessages();
    }
    ~DxWindow() {
        c.wait();
    }
};
inline int dxMain(int argc, char** argv, int mode, DxRenderOptions options = {}) {
    dxShaderFile = std::filesystem::absolute(argv[0]).parent_path() / "shaders" / "scene.hlsl";
    bool smoke = false, warp = false;
    for (int i = 1; i < argc; ++i) {
        smoke |= std::strcmp(argv[i], "--smoke") == 0;
        warp |= std::strcmp(argv[i], "--warp") == 0;
    }
    if (mode == 1 || mode == 8) {
        DxDevice c(warp);
        if (mode == 8)
            dxCompute(c);
        c.checkMessages();
        return 0;
    }
    if (!glfwInit())
        throw std::runtime_error("GLFW failed");
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    if (smoke)
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    auto window = glfwCreateWindow(960, 640, "DirectX 12 Course", nullptr, nullptr);
    if (!window)
        throw std::runtime_error("Window creation failed");
    {
        DxWindow app(window, mode, warp, options);
        int frame = 0;
        auto output = std::filesystem::path(argv[0]).parent_path() /
                      (options.captureStem.empty() ? "dx12_" + std::to_string(mode) + ".ppm"
                                                   : options.captureStem + ".ppm");
        while (!glfwWindowShouldClose(window) && (!smoke || frame < 4)) {
            glfwPollEvents();
            if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
                break;
            int w, h;
            glfwGetFramebufferSize(window, &w, &h);
            if (!w || !h) {
                glfwWaitEvents();
                continue;
            }
            app.resize(w, h);
            float time = smoke ? 1.f : float(glfwGetTime());
            if (app.scene && options.beforeFrame)
                options.beforeFrame(*app.scene, time);
            app.render(time, smoke && frame == 3 ? &output : nullptr);
            ++frame;
        }
    }
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
} // namespace course
