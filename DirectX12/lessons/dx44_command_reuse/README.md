# dx44_command_reuse · 命令存储安全重用

[54 课目录](../../COURSE.md)

## 目的与实际执行

CommandAllocator 保存录制存储，CommandList 提供录制接口。只有相关 GPU 使用结束后才能 reset allocator；command list 对象自身的 Reset 条件不应被误当成存储已安全复用。

本程序有独立实验选择与验证路径，公共实现集中在 advanced_labs.cpp。设备/API 检查、数值 compute 与图形渲染采用不同证据；不是每个程序都会弹出窗口。

## 构建和运行

```powershell
cmake --preset dx12
cmake --build --preset dx12-debug --target dx44_command_reuse --parallel 4
.\DirectX12\build\bin\Debug\dx44_command_reuse.exe
```

## 深入学习：数据、接口与执行

- [第 18 课：命令池、Allocator 与安全重置](../../chapters/18_command_storage/README.md)

### 本课的真实源码切片

来自 [DirectX12/common/advanced_labs.cpp](../../common/advanced_labs.cpp#L660)，第 660—703 行；这是现有实现的局部摘录，末尾可能处于函数中间，不是可单独编译的完整程序。

```cpp
            std::vector<uint32_t> values(64, 41 + generation);
            writeBuffer(upload.Get(), values.data(), 256);
            c.begin();
            c.cmd->CopyBufferRegion(readback.Get(), 0, upload.Get(), 0, 256);
            c.submit();
            require(readBuffer(readback.Get(), 64) == values,
                    "Allocator reset contains stale commands");
        }
        std::cout << "Verified 4 allocator/list generations after completed fence values\n";
    } else if (id == 45) {
        ComputeKernel kernel(c, options);
        kernel.execute(0, 128, 2, 3);
        auto first = readBuffer(kernel.readback.Get(), 128);
        auto second = c.buffer(4096,
                               D3D12_HEAP_TYPE_DEFAULT,
                               D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                               D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
        auto readback = c.buffer(4096, D3D12_HEAP_TYPE_READBACK, D3D12_RESOURCE_STATE_COPY_DEST);
        c.begin();
        c.cmd->SetComputeRootSignature(kernel.signature.Get());
        c.cmd->SetPipelineState(kernel.pipeline.Get());
        uint32_t values[4] = {0, 128, 5, 11};
        c.cmd->SetComputeRoot32BitConstants(0, 4, values, 0);
        c.cmd->SetComputeRootUnorderedAccessView(1, second->GetGPUVirtualAddress());
        c.cmd->SetComputeRootConstantBufferView(2, kernel.parameters->GetGPUVirtualAddress());
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
```

### 同主题跨 API 导航

| 路线 | 程序 |
| --- | --- |
| DirectX 12 | [dx44_command_reuse](README.md) |
| 对照后端 | [vk44_command_reuse](../../../Vulkan/native/lessons/vk44_command_reuse/README.md) |

### 先预测，再验证

四次录制分别写 41、42、43、44，怎样验证重置与执行链路？

参考推导：每代都等待其完成并核对全部结果，再允许下一次覆盖命令存储。仅最终值正确不能证明中间代次没有生命周期错误。

## 验证与边界

运行 `--smoke` 执行有限自动检查。数值实验读取实际 GPU 结果，图形实验检查非空图像并可查看读回。交互重建与性能还需额外证据；窗口 renderer 当前保留串行等待，双槽计算实验不会被描述为完整多帧呈现引擎。
