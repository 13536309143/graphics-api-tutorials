# dx25_timestamps · GPU 时间戳

[54 课目录](../../COURSE.md)

## 目的与实际执行

queue timestamp frequency 将 query ticks 转成时间，结果经 ResolveQueryData/readback 等待后读取。缓存 PSO 重建复用完全相同的编译字节码；Debug 重编译可能引入变化，不能仅凭源码相同断言 cache 一定兼容。

本程序有独立实验选择与验证路径，公共实现集中在 advanced_labs.cpp。设备/API 检查、数值 compute 与图形渲染采用不同证据；不是每个程序都会弹出窗口。

## 构建和运行

```powershell
cmake --preset dx12
cmake --build --preset dx12-debug --target dx25_timestamps --parallel 4
.\DirectX12\build\bin\Debug\dx25_timestamps.exe
```

## 深入学习：数据、接口与执行

- [第 53 课：GPU 时间戳、管线缓存与测量边界](../../chapters/53_measure_cache/README.md)

### 本课的真实源码切片

来自 [DirectX12/common/advanced_labs.cpp](../../common/advanced_labs.cpp#L210)，第 210—253 行；这是现有实现的局部摘录，末尾可能处于函数中间，不是可单独编译的完整程序。

```cpp
void timestamps(DxDevice& c) {
    D3D12_QUERY_HEAP_DESC desc{};
    desc.Type = D3D12_QUERY_HEAP_TYPE_TIMESTAMP;
    desc.Count = 2;
    ComPtr<ID3D12QueryHeap> heap;
    dxCheck(c.device->CreateQueryHeap(&desc, IID_PPV_ARGS(&heap)));
    auto upload = c.buffer(1024 * 1024, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ);
    auto gpu = c.buffer(1024 * 1024, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_COPY_DEST);
    auto result = c.buffer(16, D3D12_HEAP_TYPE_READBACK, D3D12_RESOURCE_STATE_COPY_DEST);
    std::vector<uint8_t> bytes(1024 * 1024);
    writeBuffer(upload.Get(), bytes.data(), bytes.size());
    c.begin();
    c.cmd->EndQuery(heap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, 0);
    c.cmd->CopyBufferRegion(gpu.Get(), 0, upload.Get(), 0, bytes.size());
    c.cmd->EndQuery(heap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, 1);
    c.cmd->ResolveQueryData(heap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, 0, 2, result.Get(), 0);
    c.submit();
    auto words = readBuffer(result.Get(), 4);
    uint64_t ticks[2]{};
    std::memcpy(ticks, words.data(), 16);
    UINT64 frequency{};
    dxCheck(c.queue->GetTimestampFrequency(&frequency));
    require(frequency != 0, "Invalid timestamp frequency");
    std::cout << "GPU copy measurement=" << double(ticks[1] - ticks[0]) * 1000 / frequency
              << " ms (not a cross-API benchmark)\n";
}
// 线性 UNORM mip 在 GPU 上由 SRV→UAV 逐级生成；sRGB 材质链用线性平均后再编码的 CPU 路径。
struct GpuTexture {
    DxDevice& c;
    ComPtr<ID3D12Resource> texture;
    std::vector<Pixels> reference;
    DXGI_FORMAT viewFormat;
    GpuTexture(
        DxDevice& context, const LabOptions& options, Pixels pixels, bool mips, bool srgb = false)
        : c(context) {
        reference = mips ? mipReference(pixels, srgb) : std::vector<Pixels>{pixels};
        const UINT levels = UINT(reference.size());
        const bool gpuMips = mips && !srgb;
        viewFormat = srgb ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM;
        D3D12_RESOURCE_DESC desc{};
        desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        desc.Width = pixels.width;
        desc.Height = pixels.height;
        desc.DepthOrArraySize = 1;
```

### 同主题跨 API 导航

| 路线 | 程序 |
| --- | --- |
| DirectX 12 | [dx25_timestamps](README.md) |
| 对照后端 | [vk25_timestamps](../../../Vulkan/native/lessons/vk25_timestamps/README.md) |

### 先预测，再验证

列出一个公平跨 API 性能实验至少需要固定的六项条件。

参考推导：工作量、分辨率、shader/材质、场景/相机、采样/呈现策略、构建配置；还应预热、重复采样并报告 CPU/GPU/整帧时间及波动。

## 验证与边界

运行 `--smoke` 执行有限自动检查。数值实验读取实际 GPU 结果，图形实验检查非空图像并可查看读回。交互重建与性能还需额外证据；窗口 renderer 当前保留串行等待，双槽计算实验不会被描述为完整多帧呈现引擎。
