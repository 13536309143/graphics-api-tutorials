# 第 46 课 · 计算管线、线程组与全局索引 · DirectX 12

[54 课目录](../../COURSE.md) · [对照精讲](../../../docs/compare/10_compute_readback.md) · [上一课](../45_msaa/README.md) · [下一课](../47_storage_tail/README.md)

## 本课目标与先修

学习目标：解释“计算管线、线程组与全局索引”解决的问题，找到实际对象和参数，运行对应实验并用数据/画面证明。先完成前一课，并能说明它交付的对象与完成条件；本课在该依赖上增加新契约。

## 为什么需要这一层

compute 不经过光栅化，可处理缓冲或图像。先计算 local size×group count 的覆盖范围，再确认每个 invocation 负责哪个元素。输出证据应是完整数值验证，不是仅打印 GPU 名。

## DirectX 12 的对象、参数与执行推导

compute PSO、root signature 和 UAV 组成同类路径，Dispatch 参数是工作组数量，不是总线程数。DEFAULT 输出再复制到 READBACK，Map 不会自动等待 GPU。

## 对照 Vulkan 原生：相同问题的另一种分工

compute pipeline 使用独立 shader stage，复用 descriptor/pipeline layout 概念。原课以 64×4 组计算 256 项 i*i+1；buffer 写入经 host 可见依赖并等待完成后读取。

## 关键参数与可检查不变量

| 检查项 | Vulkan 表达 | DX12 表达 | 不变量 / 证据 |
| --- | --- | --- | --- |
| 工作范围 | 64 线程 × 4 组 | 相同 64×4 | 256 项公式逐项验证 |
| 可写接口 | storage buffer descriptor | root UAV | 不走光栅化阶段 |
| 读回完成 | host-read 依赖 + fence | copy READBACK + fence | Map 不触发自动 GPU 等待 |

先为每项指出源码中的创建/绑定位置，以及证明它成立的实际日志、字节或图像。数值中的设备限制以查询为准；固定实验尺寸不当作所有设备的上限。

## nvpro_core2 框架对照

nv05 由 Context 提供设备/队列并复用 computeSquares。它不创建普通窗口循环，成功结果在运行输出中查看。

这部分是原生 Vulkan 各课的框架对照，框架可运行入口仍为原有六课。如果本节需要新接入工作，会按正文说明；不把原生实验当作已完成的框架实现。

## 运行与实际检查

| 路线 | 实际实验 |
| --- | --- |
| DirectX 12 | [dx08_compute_uav](../../lessons/dx08_compute_uav/README.md) |
| Vulkan 原生 | [vk08_compute_readback](../../../Vulkan/native/lessons/vk08_compute_readback/README.md) |

从合集根目录执行；普通图形程序 Esc 退出，设备/计算/诊断程序输出结果后结束。从“开始学习.cmd”进入本页时，表格可直接点击运行。

```powershell
cmake --preset dx12
cmake --build --preset dx12-debug --target dx08_compute_uav --parallel 4
.\DirectX12\build\bin\Debug\dx08_compute_uav.exe
ctest --preset dx12-debug -R "^dx08_compute_uav$" --output-on-failure
```

检查三个层次：创建/绑定/提交的接口约束；数据读回或目标画面；资源使用结束后的回收。诊断实验用合法信息或查询证明渠道可用，不故意保留 API 错误。图形 smoke 的像素检查与数值实验的逐项检查不同，具体实现见源码。

## 关键实现：从当前源码定位

来自 [DirectX12/common/dx12_course.hpp](../../common/dx12_course.hpp#L197)，第 197—240 行。下列是当前仓库的局部切片，可能在函数中间结束；完整上下文请打开文件，切片不作为单独可编译程序。

```cpp
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
```

阅读时按“前置对象 → 参数/布局 → 录制/调用 → 完成/可见性 → 释放”五步标注。共享 helper 减少重复代码，但不会取消每课的 API 契约。

## 受控修改实验

为什么 shader 用组内索引而不是全局索引，会让四组写到同一段数据？

先记录预测，再只改一个因素；保留基线与修改后的输出。不要同时改变 shader、资源范围和同步，这样即使结果变好也无法归因。

### 参考推导与通过标准

每组的组内索引都从零开始；必须组合 group 与 local，或使用全局内建，才能覆盖完整数组。

## 常见失败与排查顺序

把 dispatch(256,1,1) 当成 256 个线程，在 local size=64 时实际发出 16384 个调用，若无边界检查会越界。

先看第一条错误和当前执行路径，再看实际数据/绑定，最后看 shader 与画面。需要 GPU 证据时抓取正常交互帧；仅四帧的 smoke 适合自动检查，不适合手动抓帧。

## 自测与本课交付

能否不看源码回答：为什么 shader 用组内索引而不是全局索引，会让四组写到同一段数据？

提交一份简短记录：输入与假设、唯一修改、预期、实际证据、原因、是否恢复基线。如果只能复述函数名而说不清何时允许访问/释放，先回到本课的数据流。

## 官方阅读位置与当前工程差异

[指定 Vulkan 教程对应位置](https://docs.vulkan.net.cn/tutorial/latest/11_Compute_Shader.html)。DX12 路线按同一概念顺序建立映射，其对象与约束以 Microsoft 文档和本后端代码为准。49—54 课包含在基础/compute 之后加入的工程扩展。

[版本与资料说明](../../../docs/REFERENCES.md) · [调试](../../../docs/DEBUGGING.md) · [功能完成度](../../../docs/FEATURE_MATRIX.md)
