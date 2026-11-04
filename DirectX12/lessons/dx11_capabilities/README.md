# dx11_capabilities · 能力与格式查询

[54 课目录](../../COURSE.md)

## 目的与实际执行

DXGI factory/adapter 处理平台与设备选择，ID3D12Device 创建资源与管线，CommandQueue 执行命令。root signature 描述资源接口，PSO 描述着色器与固定状态；它们不等价于整套 Renderer 或操作系统窗口。

adapter 提供设备描述，feature query 提供具体能力。ResourceBindingTier、ResourceHeapTier、FORMAT_SUPPORT 和 MSAA quality levels 不是同一个等级。feature level 是一组基础要求，不能代替对可选能力的查询。

本程序有独立实验选择与验证路径，公共实现集中在 advanced_labs.cpp。设备/API 检查、数值 compute 与图形渲染采用不同证据；不是每个程序都会弹出窗口。

## 构建和运行

```powershell
cmake --preset dx12
cmake --build --preset dx12-debug --target dx11_capabilities --parallel 4
.\DirectX12\build\bin\Debug\dx11_capabilities.exe
```

## 深入学习：数据、接口与执行

- [第 01 课：图形 API 的职责与课程地图](../../chapters/01_architecture/README.md)
- [第 06 课：物理设备、Adapter 与格式能力](../../chapters/06_physical_device/README.md)

### 本课的真实源码切片

来自 [DirectX12/common/advanced_labs.cpp](../../common/advanced_labs.cpp#L129)，第 129—172 行；这是现有实现的局部摘录，末尾可能处于函数中间，不是可单独编译的完整程序。

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

### 同主题跨 API 导航

| 路线 | 程序 |
| --- | --- |
| DirectX 12 | [dx11_capabilities](README.md) |
| 对照后端 | [vk11_capabilities](../../../Vulkan/native/lessons/vk11_capabilities/README.md) |

### 先预测，再验证

画出一个三角形所需的五类对象，并说明哪些对象保存字节、哪些只保存解释或引用。

参考推导：Buffer/Image 保存内容，ImageView/descriptor/布局解释或引用内容，pipeline 约定执行方式；Queue 执行已录制工作。引用不会复制资源，也不会延长 GPU 资源的合法使用期。

写出本项目基础与 MSAA 实验各自的能力清单，并标明查询位置。

参考推导：基础至少需要图形/计算、RGBA8、深度、常量范围；MSAA 还要颜色和深度共同支持同一样本数。单独颜色支持 4x 不足以建立匹配附件。

## 验证与边界

运行 `--smoke` 执行有限自动检查。数值实验读取实际 GPU 结果，图形实验检查非空图像并可查看读回。交互重建与性能还需额外证据；窗口 renderer 当前保留串行等待，双槽计算实验不会被描述为完整多帧呈现引擎。
