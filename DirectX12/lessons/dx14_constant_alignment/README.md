# dx14_constant_alignment · 常量缓冲对齐

[54 课目录](../../COURSE.md)

## 目的与实际执行

GetResourceAllocationInfo 解释实际分配大小/对齐，UPLOAD/DEFAULT/READBACK 表达常见用途。GetCustomHeapProperties 可以帮助理解架构映射，但不能随意绕过 heap/resource 规则。CBV 的 256 字节地址对齐与普通 buffer 分配对齐也不是同一要求。

HLSL column_major 与 mul(M,v) 配合 GLM。root CBV 的 GPU 地址需要 256 字节对齐，实际帧矩阵仍只有 64 字节；根常量按 DWORD 更新。不要把所有 C++ struct 都假定与 cbuffer 打包自动一致。

本程序有独立实验选择与验证路径，公共实现集中在 advanced_labs.cpp。设备/API 检查、数值 compute 与图形渲染采用不同证据；不是每个程序都会弹出窗口。

## 构建和运行

```powershell
cmake --preset dx12
cmake --build --preset dx12-debug --target dx14_constant_alignment --parallel 4
.\DirectX12\build\bin\Debug\dx14_constant_alignment.exe
```

## 深入学习：数据、接口与执行

- [第 25 课：内存类型、Heap 与分配要求](../../chapters/25_memory/README.md)
- [第 28 课：常量结构、矩阵存储与对齐](../../chapters/28_uniform_layout/README.md)

### 本课的真实源码切片

来自 [DirectX12/common/advanced_labs.cpp](../../common/advanced_labs.cpp#L56)，第 56—99 行；这是现有实现的局部摘录，末尾可能处于函数中间，不是可单独编译的完整程序。

```cpp
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
```

### 同主题跨 API 导航

| 路线 | 程序 |
| --- | --- |
| DirectX 12 | [dx14_constant_alignment](README.md) |
| 对照后端 | [vk14_constant_alignment](../../../Vulkan/native/lessons/vk14_constant_alignment/README.md) |

### 先预测，再验证

比较请求 257 字节、返回的 allocation size、常量槽 stride 三者，解释不同来源。

参考推导：257 是应用内容大小，allocation size 来自内存分配约束，常量 stride 来自接口偏移对齐。三者可以不同而同时正确。

在 CPU 打印 sizeof/offsetof，再通过 GPU 计算验证第三个常量槽的 factor/bias。

参考推导：CPU 布局断言检查内存，GPU 结果检查接口选择与解码；两份证据互补。第三槽得到 i*7+13，才说明该范围被正确使用。

## 验证与边界

运行 `--smoke` 执行有限自动检查。数值实验读取实际 GPU 结果，图形实验检查非空图像并可查看读回。交互重建与性能还需额外证据；窗口 renderer 当前保留串行等待，双槽计算实验不会被描述为完整多帧呈现引擎。
