# dx15_dynamic_binding · 动态常量范围

[54 课目录](../../COURSE.md)

## 目的与实际执行

同一 UPLOAD buffer 中每槽 256 字节，以 root CBV GPU 地址加 slot*256 选择。没有 Vulkan 式 dynamic descriptor type，但要解决相同的对齐、范围和在途更新问题。

本程序有独立实验选择与验证路径，公共实现集中在 advanced_labs.cpp。设备/API 检查、数值 compute 与图形渲染采用不同证据；不是每个程序都会弹出窗口。

## 构建和运行

```powershell
cmake --preset dx12
cmake --build --preset dx12-debug --target dx15_dynamic_binding --parallel 4
.\DirectX12\build\bin\Debug\dx15_dynamic_binding.exe
```

## 深入学习：数据、接口与执行

- [第 31 课：动态 UBO 与 Root CBV 地址选择](../../chapters/31_dynamic_offsets/README.md)

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
| DirectX 12 | [dx15_dynamic_binding](README.md) |
| 对照后端 | [vk15_dynamic_binding](../../../Vulkan/native/lessons/vk15_dynamic_binding/README.md) |

### 先预测，再验证

把三个槽的参数互换而不改变 shader，预测结果应如何变化。

参考推导：输出应随绑定范围改变。若 CPU 内容变了而输出仍来自旧槽，检查实际 bind offset/address，而不是重新编译 shader。

## 验证与边界

运行 `--smoke` 执行有限自动检查。数值实验读取实际 GPU 结果，图形实验检查非空图像并可查看读回。交互重建与性能还需额外证据；窗口 renderer 当前保留串行等待，双槽计算实验不会被描述为完整多帧呈现引擎。
