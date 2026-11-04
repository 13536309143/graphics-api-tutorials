# dx16_push_constants · 小常量计算

[54 课目录](../../COURSE.md)

## 目的与实际执行

四个 root constants 映射 b0，root UAV 映射 u0，root CBV 映射 b1。root parameter 的数组下标与 HLSL register 不是一套编号；更新 root constants 使用 DWORD 数而不是字节数。

root signature 组合 root constants、root descriptor 和 descriptor table。parameter 2 可以指 t0，register space 又是独立维度。显式说明接口约定，才能检查 shader 与调用是否兼容。

root constants 使用 32 位值数量与偏移，对应 HLSL cbuffer。20 DWORD 的 Draw 与 4 DWORD 的 Operation 不同；root signature 总成本也限制接口设计。

本程序有独立实验选择与验证路径，公共实现集中在 advanced_labs.cpp。设备/API 检查、数值 compute 与图形渲染采用不同证据；不是每个程序都会弹出窗口。

## 构建和运行

```powershell
cmake --preset dx12
cmake --build --preset dx12-debug --target dx16_push_constants --parallel 4
.\DirectX12\build\bin\Debug\dx16_push_constants.exe
```

## 深入学习：数据、接口与执行

- [第 16 课：Pipeline Layout、根签名与小常量](../../chapters/16_pipeline_interface/README.md)
- [第 29 课：描述符布局与资源接口声明](../../chapters/29_descriptor_layout/README.md)
- [第 32 课：Push/Root Constants 的单位与边界](../../chapters/32_per_draw_constants/README.md)

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
| DirectX 12 | [dx16_push_constants](README.md) |
| 对照后端 | [vk16_push_constants](../../../Vulkan/native/lessons/vk16_push_constants/README.md) |

### 先预测，再验证

把 factor/bias 改为 9/17，如何证明 GPU 确实收到了新常量？

参考推导：逐项检查 result[i]=9*i+17，而不是只看 CPU 断点中的变量。结果错时核对参数顺序、大小、阶段和入口。

为一个每帧相机、每材质纹理、每对象 model 的场景设计更新频率划分。

参考推导：按帧更新 Frame，按材质选择纹理引用，按 draw 更新 model 小常量。不要每 draw 重建全部布局，也不把大型场景数据塞进 push/root constants。

加入第二个对象，仅改变它的 model，说明不需要重建哪些长期对象。

参考推导：静态几何、纹理、layout 和同一兼容 pipeline 可以复用，只在对应 draw 前更新小常量。更改不可动态状态或 shader 才可能要求另一 pipeline。

## 验证与边界

运行 `--smoke` 执行有限自动检查。数值实验读取实际 GPU 结果，图形实验检查非空图像并可查看读回。交互重建与性能还需额外证据；窗口 renderer 当前保留串行等待，双槽计算实验不会被描述为完整多帧呈现引擎。
