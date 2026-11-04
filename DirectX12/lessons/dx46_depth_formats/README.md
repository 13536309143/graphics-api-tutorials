# dx46_depth_formats · 深度格式支持

[54 课目录](../../COURSE.md)

## 目的与实际执行

FORMAT_SUPPORT 的 depth-stencil 支持与 MULTISAMPLE_QUALITY_LEVELS 分别查询。DSV 的格式/维度和 PSO 的 DSVFormat、sample count 要一致，resource flags 还需允许深度用途。

本程序有独立实验选择与验证路径，公共实现集中在 advanced_labs.cpp。设备/API 检查、数值 compute 与图形渲染采用不同证据；不是每个程序都会弹出窗口。

## 构建和运行

```powershell
cmake --preset dx12
cmake --build --preset dx12-debug --target dx46_depth_formats --parallel 4
.\DirectX12\build\bin\Debug\dx46_depth_formats.exe
```

## 深入学习：数据、接口与执行

- [第 39 课：深度格式、用途与样本支持](../../chapters/39_depth_formats/README.md)

### 本课的真实源码切片

来自 [DirectX12/common/advanced_labs.cpp](../../common/advanced_labs.cpp#L694)，第 694—737 行；这是现有实现的局部摘录，末尾可能处于函数中间，不是可单独编译的完整程序。

```cpp
        require(readBuffer(kernel.readback.Get(), 128) == first,
                "Prior output was unexpectedly changed");
        std::cout << "Verified root descriptor replacement after prior GPU use completed\n";
    } else if (id == 46) {
        for (auto format :
             {DXGI_FORMAT_D32_FLOAT, DXGI_FORMAT_D24_UNORM_S8_UINT, DXGI_FORMAT_D16_UNORM}) {
            D3D12_FEATURE_DATA_FORMAT_SUPPORT q{format};
            dxCheck(c.device->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &q, sizeof(q)));
            std::cout << "depth format=" << format << ", support1=" << q.Support1 << '\n';
        }
    } else if (id == 47) {
        D3D12_RESOURCE_DESC desc{};
        desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        desc.Width = 257;
        desc.Height = 1;
        desc.DepthOrArraySize = 1;
        desc.MipLevels = 1;
        desc.SampleDesc.Count = 1;
        desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        auto allocation = c.device->GetResourceAllocationInfo(0, 1, &desc);
        require(allocation.SizeInBytes >= 257, "Allocation too small");
        auto upload = c.device->GetCustomHeapProperties(0, D3D12_HEAP_TYPE_UPLOAD);
        std::cout << "requested=257, allocation=" << allocation.SizeInBytes
                  << ", alignment=" << allocation.Alignment
                  << ", CPUPageProperty=" << upload.CPUPageProperty << '\n';
    } else if (id == 48) {
        auto initial = c.fence->GetCompletedValue();
        dxCheck(c.queue->Signal(c.fence.Get(), ++c.value));
        waitValue(c, c.value);
        require(c.fence->GetCompletedValue() > initial, "Fence counter failed to advance");
        dxCheck(c.queue->Signal(c.fence.Get(), ++c.value));
        waitValue(c, c.value);
        std::cout << "Verified monotonic fence reuse; DX12 fence is not reset like a binary Vulkan "
                     "fence\n";
    } else if (id == 51) {
        static_assert(sizeof(Frame) == 64 && sizeof(Draw) == 80 && offsetof(Draw, settings) == 64);
        ComputeKernel k(c, options);
        k.execute(1, 128, 0, 0, 2);
        auto r = readBuffer(k.readback.Get(), 128);
        for (uint32_t i = 0; i < 128; ++i)
            require(r[i] == i * 7 + 13, "CBV vector layout mismatch");
        std::cout << "Verified Frame=64, Draw=80, offset=64 and 256-byte-aligned CBV selection\n";
    } else if (id == 54) {
        capabilities(c);
```

### 同主题跨 API 导航

| 路线 | 程序 |
| --- | --- |
| DirectX 12 | [dx46_depth_formats](README.md) |
| 对照后端 | [vk46_depth_formats](../../../Vulkan/native/lessons/vk46_depth_formats/README.md) |

### 先预测，再验证

为颜色和深度选择共同支持的样本数，写出如何取合法交集。

参考推导：Vulkan 检查颜色/深度 sample flags 的交集并结合实际格式；DX12 分别查询对应格式的指定样本数，只有两者支持才能组合。

## 验证与边界

运行 `--smoke` 执行有限自动检查。数值实验读取实际 GPU 结果，图形实验检查非空图像并可查看读回。交互重建与性能还需额外证据；窗口 renderer 当前保留串行等待，双槽计算实验不会被描述为完整多帧呈现引擎。
