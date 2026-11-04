# dx28_compute_tail · 非整组尾部计算

[54 课目录](../../COURSE.md)

## 目的与实际执行

RWStructuredBuffer<uint> 的结构元素大小与资源容量匹配；root UAV 选择地址但不会携带自动上界保护。即使资源足够大，逻辑上未写的元素也不能当成正确结果。

本程序有独立实验选择与验证路径，公共实现集中在 advanced_labs.cpp。设备/API 检查、数值 compute 与图形渲染采用不同证据；不是每个程序都会弹出窗口。

## 构建和运行

```powershell
cmake --preset dx12
cmake --build --preset dx12-debug --target dx28_compute_tail --parallel 4
.\DirectX12\build\bin\Debug\dx28_compute_tail.exe
```

## 深入学习：数据、接口与执行

- [第 47 课：Storage/UAV、边界与 257 项实验](../../chapters/47_storage_tail/README.md)

### 本课的真实源码切片

来自 [DirectX12/common/advanced_labs.cpp](../../common/advanced_labs.cpp#L100)，第 100—143 行；这是现有实现的局部摘录，末尾可能处于函数中间，不是可单独编译的完整程序。

```cpp
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
```

### 同主题跨 API 导航

| 路线 | 程序 |
| --- | --- |
| DirectX 12 | [dx28_compute_tail](README.md) |
| 对照后端 | [vk28_compute_tail](../../../Vulkan/native/lessons/vk28_compute_tail/README.md) |

### 先预测，再验证

只把 CPU 验证改成 257 而仍分配 256 项，会产生什么问题？

参考推导：可能读取越界；只扩容量却不扩组数则尾项未写。正确验收不能用忽略最后一项来让测试通过。

## 验证与边界

运行 `--smoke` 执行有限自动检查。数值实验读取实际 GPU 结果，图形实验检查非空图像并可查看读回。交互重建与性能还需额外证据；窗口 renderer 当前保留串行等待，双槽计算实验不会被描述为完整多帧呈现引擎。
