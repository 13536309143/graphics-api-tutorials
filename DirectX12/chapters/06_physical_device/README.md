# 第 06 课 · 物理设备、Adapter 与格式能力 · DirectX 12

[54 课目录](../../COURSE.md) · [对照精讲](../../../docs/compare/01_context_device.md) · [上一课](../05_validation/README.md) · [下一课](../07_queues/README.md)

## 本课目标与先修

学习目标：解释“物理设备、Adapter 与格式能力”解决的问题，找到实际对象和参数，运行对应实验并用数据/画面证明。先完成前一课，并能说明它交付的对象与完成条件；本课在该依赖上增加新契约。

## 为什么需要这一层

选择设备不是仅比较显存大小。要检查任务所需的队列、格式、样本数、资源限制与特性，并报告不满足哪项。把“最贵的显卡”当成“符合当前契约的设备”会掩盖实际依赖。

## DirectX 12 的对象、参数与执行推导

adapter 提供设备描述，feature query 提供具体能力。ResourceBindingTier、ResourceHeapTier、FORMAT_SUPPORT 和 MSAA quality levels 不是同一个等级。feature level 是一组基础要求，不能代替对可选能力的查询。

## 对照 Vulkan 原生：相同问题的另一种分工

PhysicalDeviceProperties 提供 limits，MemoryProperties 描述 heap/type。格式还要单独查 optimal tiling features。实验检查 80 字节 Draw 的 push constant 容量及 D32 深度用途，同时输出 UBO 对齐、工作组限制和内存类型。

## 关键参数与可检查不变量

| 检查项 | Vulkan 表达 | DX12 表达 | 不变量 / 证据 |
| --- | --- | --- | --- |
| 小常量能力 | maxPushConstantsSize >= 80 | root signature 总成本合法 | 检查实际使用范围，而非只读设备名称 |
| 格式支持 | optimalTilingFeatures | FORMAT_SUPPORT / MSAA query | 格式、用途和样本数共同成立 |
| 内存属性 | heap/type 与 flags | heap tier / type | GPU 型号或显存大小不是全部契约 |

先为每项指出源码中的创建/绑定位置，以及证明它成立的实际日志、字节或图像。数值中的设备限制以查询为准；固定实验尺寸不当作所有设备的上限。

## nvpro_core2 框架对照

Context 的设备选择结果应与需求列表一起审查。框架选择到一个可用 device 后，新的效果仍可能要求额外格式或 sample count。

这部分是原生 Vulkan 各课的框架对照，框架可运行入口仍为原有六课。如果本节需要新接入工作，会按正文说明；不把原生实验当作已完成的框架实现。

## 运行与实际检查

| 路线 | 实际实验 |
| --- | --- |
| DirectX 12 | [dx11_capabilities](../../lessons/dx11_capabilities/README.md) |
| Vulkan 原生 | [vk11_capabilities](../../../Vulkan/native/lessons/vk11_capabilities/README.md) |

从合集根目录执行；普通图形程序 Esc 退出，设备/计算/诊断程序输出结果后结束。从“开始学习.cmd”进入本页时，表格可直接点击运行。

```powershell
cmake --preset dx12
cmake --build --preset dx12-debug --target dx11_capabilities --parallel 4
.\DirectX12\build\bin\Debug\dx11_capabilities.exe
ctest --preset dx12-debug -R "^dx11_capabilities$" --output-on-failure
```

检查三个层次：创建/绑定/提交的接口约束；数据读回或目标画面；资源使用结束后的回收。诊断实验用合法信息或查询证明渠道可用，不故意保留 API 错误。图形 smoke 的像素检查与数值实验的逐项检查不同，具体实现见源码。

## 关键实现：从当前源码定位

来自 [DirectX12/common/advanced_labs.cpp](../../common/advanced_labs.cpp#L129)，第 129—172 行。下列是当前仓库的局部切片，可能在函数中间结束；完整上下文请打开文件，切片不作为单独可编译程序。

```cpp
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
        c.device->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &ms, sizeof(ms)));
    std::cout << "RGBA8 4x MSAA quality levels=" << ms.NumQualityLevels << '\n';
}
// UPLOAD 写入后复制到 DEFAULT，再转 COPY_SOURCE 并读回；upload 保留到 fence 完成。
void stagingCopy(DxDevice& c) {
    std::vector<uint32_t> original(257);
    for (uint32_t i = 0; i < 257; ++i)
        original[i] = i * 17 + 9;
    auto upload = c.buffer(257 * 4, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ);
    auto gpu = c.buffer(257 * 4, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_COPY_DEST);
    auto readback = c.buffer(257 * 4, D3D12_HEAP_TYPE_READBACK, D3D12_RESOURCE_STATE_COPY_DEST);
    writeBuffer(upload.Get(), original.data(), original.size() * 4);
    c.begin();
    c.cmd->CopyBufferRegion(gpu.Get(), 0, upload.Get(), 0, 257 * 4);
    auto b =
        transition(gpu.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COPY_SOURCE);
    c.cmd->ResourceBarrier(1, &b);
    c.cmd->CopyBufferRegion(readback.Get(), 0, gpu.Get(), 0, 257 * 4);
    c.submit();
    require(readBuffer(readback.Get(), 257) == original, "DEFAULT staging-copy result mismatch");
    std::cout << "Verified 257 DEFAULT round-trip words\n";
}
// 两组 allocator/list 与可变资源交替提交；等待的是本槽上次使用，不是每次提交后立即全局等待。
void frameRing(DxDevice& c, const LabOptions& options) {
    ComputeKernel kernels[2] = {ComputeKernel(c, options), ComputeKernel(c, options)};
    ComPtr<ID3D12CommandAllocator> allocators[2];
    ComPtr<ID3D12GraphicsCommandList> commands[2];
    UINT64 completed[2]{};
    int previous[2] = {-1, -1};
```

阅读时按“前置对象 → 参数/布局 → 录制/调用 → 完成/可见性 → 释放”五步标注。共享 helper 减少重复代码，但不会取消每课的 API 契约。

## 受控修改实验

写出本项目基础与 MSAA 实验各自的能力清单，并标明查询位置。

先记录预测，再只改一个因素；保留基线与修改后的输出。不要同时改变 shader、资源范围和同步，这样即使结果变好也无法归因。

### 参考推导与通过标准

基础至少需要图形/计算、RGBA8、深度、常量范围；MSAA 还要颜色和深度共同支持同一样本数。单独颜色支持 4x 不足以建立匹配附件。

## 常见失败与排查顺序

能力查询失败时应报告具体条件，不能把“不支持”直接解释成文件或 shader 语法错误。

先看第一条错误和当前执行路径，再看实际数据/绑定，最后看 shader 与画面。需要 GPU 证据时抓取正常交互帧；仅四帧的 smoke 适合自动检查，不适合手动抓帧。

## 自测与本课交付

能否不看源码回答：写出本项目基础与 MSAA 实验各自的能力清单，并标明查询位置。

提交一份简短记录：输入与假设、唯一修改、预期、实际证据、原因、是否恢复基线。如果只能复述函数名而说不清何时允许访问/释放，先回到本课的数据流。

## 官方阅读位置与当前工程差异

[指定 Vulkan 教程对应位置](https://docs.vulkan.net.cn/tutorial/latest/03_Drawing_a_triangle/00_Setup/03_Physical_devices_and_queue_families.html)。DX12 路线按同一概念顺序建立映射，其对象与约束以 Microsoft 文档和本后端代码为准。49—54 课包含在基础/compute 之后加入的工程扩展。

[版本与资料说明](../../../docs/REFERENCES.md) · [调试](../../../docs/DEBUGGING.md) · [功能完成度](../../../docs/FEATURE_MATRIX.md)
