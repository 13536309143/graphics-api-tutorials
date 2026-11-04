# dx45_descriptor_update · 资源引用替换

[54 课目录](../../COURSE.md)

## 目的与实际执行

计算实验用 root UAV 地址替换资源引用，纹理实验则通过 shader-visible heap 与 descriptor table 展示 heap 模型。二者不是同一接口，课程明确标注；CPU descriptor 的构建与 GPU handle 的绑定各有位置。

本程序有独立实验选择与验证路径，公共实现集中在 advanced_labs.cpp。设备/API 检查、数值 compute 与图形渲染采用不同证据；不是每个程序都会弹出窗口。

## 构建和运行

```powershell
cmake --preset dx12
cmake --build --preset dx12-debug --target dx45_descriptor_update --parallel 4
.\DirectX12\build\bin\Debug\dx45_descriptor_update.exe
```

## 深入学习：数据、接口与执行

- [第 30 课：描述符池、Heap 与资源引用更新](../../chapters/30_descriptor_instances/README.md)

### 本课的真实源码切片

来自 [DirectX12/common/advanced_labs.cpp](../../common/advanced_labs.cpp#L685)，第 685—728 行；这是现有实现的局部摘录，末尾可能处于函数中间，不是可单独编译的完整程序。

```cpp
        c.cmd->Dispatch(2, 1, 1);
        auto b = transition(
            second.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);
        c.cmd->ResourceBarrier(1, &b);
        c.cmd->CopyBufferRegion(readback.Get(), 0, second.Get(), 0, 4096);
        c.submit();
        auto result = readBuffer(readback.Get(), 128);
        for (uint32_t i = 0; i < 128; ++i)
            require(result[i] == i * 5 + 11, "Root UAV replacement mismatch");
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
```

### 同主题跨 API 导航

| 路线 | 程序 |
| --- | --- |
| DirectX 12 | [dx45_descriptor_update](README.md) |
| 对照后端 | [vk45_descriptor_update](../../../Vulkan/native/lessons/vk45_descriptor_update/README.md) |

### 先预测，再验证

修改绑定到 B 后，为什么还要检查 A？

参考推导：这能发现 CPU 误写旧资源、错误绑定或读回来源混淆。只看 B 的某个值可能被旧内容偶然满足。

## 验证与边界

运行 `--smoke` 执行有限自动检查。数值实验读取实际 GPU 结果，图形实验检查非空图像并可查看读回。交互重建与性能还需额外证据；窗口 renderer 当前保留串行等待，双槽计算实验不会被描述为完整多帧呈现引擎。
