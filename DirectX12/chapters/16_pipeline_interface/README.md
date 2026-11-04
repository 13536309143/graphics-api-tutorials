# 第 16 课 · Pipeline Layout、根签名与小常量 · DirectX 12

[54 课目录](../../COURSE.md) · [对照精讲](../../../docs/compare/05_binding_constants.md) · [上一课](../15_rendering_scope/README.md) · [下一课](../17_targets/README.md)

## 本课目标与先修

学习目标：解释“Pipeline Layout、根签名与小常量”解决的问题，找到实际对象和参数，运行对应实验并用数据/画面证明。先完成前一课，并能说明它交付的对象与完成条件；本课在该依赖上增加新契约。

## 为什么需要这一层

管线接口描述 shader 能访问什么及其布局，不存放整套数据。小块逐 draw/dispatch 参数适合轻量更新，大量场景/骨骼数据则应进入缓冲。接口形状要与编译代码一致。

## DirectX 12 的对象、参数与执行推导

四个 root constants 映射 b0，root UAV 映射 u0，root CBV 映射 b1。root parameter 的数组下标与 HLSL register 不是一套编号；更新 root constants 使用 DWORD 数而不是字节数。

## 对照 Vulkan 原生：相同问题的另一种分工

实验用 16 字节 push constants 传 mode/count/factor/bias。pipeline layout 的 range、stageFlags 与 vkCmdPushConstants 的更新范围必须兼容。即使数据很小，也不能忽略设备 maxPushConstantsSize。

## 关键参数与可检查不变量

| 检查项 | Vulkan 表达 | DX12 表达 | 不变量 / 证据 |
| --- | --- | --- | --- |
| 计算小常量 | 16 bytes push | 4 DWORD root constants | 参数顺序 mode/count/factor/bias 一致 |
| 图形小常量 | Draw 80 bytes | Draw 20 DWORD | 不同 pipeline 接口不混用 |
| 阶段/范围 | stageFlags + byte range | root parameter + DWORD offset | 范围正确且覆盖 shader 实际读取 |

先为每项指出源码中的创建/绑定位置，以及证明它成立的实际日志、字节或图像。数值中的设备限制以查询为准；固定实验尺寸不当作所有设备的上限。

## nvpro_core2 框架对照

Framework 不改变 pipeline layout 的二进制约定。nvpro 视口复用相同 SceneVk，学习时沿实际 push 更新进入 shader。

这部分是原生 Vulkan 各课的框架对照，框架可运行入口仍为原有六课。如果本节需要新接入工作，会按正文说明；不把原生实验当作已完成的框架实现。

## 运行与实际检查

| 路线 | 实际实验 |
| --- | --- |
| DirectX 12 | [dx16_push_constants](../../lessons/dx16_push_constants/README.md) |
| Vulkan 原生 | [vk16_push_constants](../../../Vulkan/native/lessons/vk16_push_constants/README.md) |

从合集根目录执行；普通图形程序 Esc 退出，设备/计算/诊断程序输出结果后结束。从“开始学习.cmd”进入本页时，表格可直接点击运行。

```powershell
cmake --preset dx12
cmake --build --preset dx12-debug --target dx16_push_constants --parallel 4
.\DirectX12\build\bin\Debug\dx16_push_constants.exe
ctest --preset dx12-debug -R "^dx16_push_constants$" --output-on-failure
```

检查三个层次：创建/绑定/提交的接口约束；数据读回或目标画面；资源使用结束后的回收。诊断实验用合法信息或查询证明渠道可用，不故意保留 API 错误。图形 smoke 的像素检查与数值实验的逐项检查不同，具体实现见源码。

## 关键实现：从当前源码定位

来自 [DirectX12/common/advanced_labs.cpp](../../common/advanced_labs.cpp#L100)，第 100—143 行。下列是当前仓库的局部切片，可能在函数中间结束；完整上下文请打开文件，切片不作为单独可编译程序。

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

阅读时按“前置对象 → 参数/布局 → 录制/调用 → 完成/可见性 → 释放”五步标注。共享 helper 减少重复代码，但不会取消每课的 API 契约。

## 受控修改实验

把 factor/bias 改为 9/17，如何证明 GPU 确实收到了新常量？

先记录预测，再只改一个因素；保留基线与修改后的输出。不要同时改变 shader、资源范围和同步，这样即使结果变好也无法归因。

### 参考推导与通过标准

逐项检查 result[i]=9*i+17，而不是只看 CPU 断点中的变量。结果错时核对参数顺序、大小、阶段和入口。

## 常见失败与排查顺序

把 16 字节误写为 16 个 DWORD，或用错阶段范围，会破坏接口；不要通过扩大所有范围掩盖错误。

先看第一条错误和当前执行路径，再看实际数据/绑定，最后看 shader 与画面。需要 GPU 证据时抓取正常交互帧；仅四帧的 smoke 适合自动检查，不适合手动抓帧。

## 自测与本课交付

能否不看源码回答：把 factor/bias 改为 9/17，如何证明 GPU 确实收到了新常量？

提交一份简短记录：输入与假设、唯一修改、预期、实际证据、原因、是否恢复基线。如果只能复述函数名而说不清何时允许访问/释放，先回到本课的数据流。

## 官方阅读位置与当前工程差异

[指定 Vulkan 教程对应位置](https://docs.vulkan.net.cn/tutorial/latest/03_Drawing_a_triangle/02_Graphics_pipeline_basics/04_Conclusion.html)。DX12 路线按同一概念顺序建立映射，其对象与约束以 Microsoft 文档和本后端代码为准。49—54 课包含在基础/compute 之后加入的工程扩展。

[版本与资料说明](../../../docs/REFERENCES.md) · [调试](../../../docs/DEBUGGING.md) · [功能完成度](../../../docs/FEATURE_MATRIX.md)
