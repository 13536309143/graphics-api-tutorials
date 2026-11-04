# dx42_shader_inspection · 着色器产物检查

[54 课目录](../../COURSE.md)

## 目的与实际执行

D3DCompiler 生成 SM5.1 字节码；实验通过反射确认 ComputeMain 的工作组为 64×1×1 并检查资源绑定。Debug 产物可能带不同调试数据，管线缓存重建应复用完全一致的字节码，不能假定同源码重编译必然字节相同。 本例使用 SM5.x 的 ID3D11ShaderReflection 反射接口，这不表示渲染后端改成了 D3D11。

本程序有独立实验选择与验证路径，公共实现集中在 advanced_labs.cpp。设备/API 检查、数值 compute 与图形渲染采用不同证据；不是每个程序都会弹出窗口。

## 构建和运行

```powershell
cmake --preset dx12
cmake --build --preset dx12-debug --target dx42_shader_inspection --parallel 4
.\DirectX12\build\bin\Debug\dx42_shader_inspection.exe
```

## 深入学习：数据、接口与执行

- [第 13 课：着色器编译、模块与反射](../../chapters/13_shader_modules/README.md)

### 本课的真实源码切片

来自 [DirectX12/common/advanced_labs.cpp](../../common/advanced_labs.cpp#L639)，第 639—682 行；这是现有实现的局部摘录，末尾可能处于函数中间，不是可单独编译的完整程序。

```cpp
    } else if (id == 39) {
        auto desc = c.queue->GetDesc();
        require(desc.Type == D3D12_COMMAND_LIST_TYPE_DIRECT, "Expected direct queue");
        std::cout << "queue type=" << desc.Type << ", priority=" << desc.Priority
                  << ", flags=" << desc.Flags << ", nodeMask=" << desc.NodeMask << '\n';
    } else if (id == 42) {
        auto shader = labShader(options, "ComputeMain");
        ComPtr<ID3D11ShaderReflection> reflection;
        dxCheck(D3DReflect(
            shader->GetBufferPointer(), shader->GetBufferSize(), IID_PPV_ARGS(&reflection)));
        UINT x{}, y{}, z{};
        reflection->GetThreadGroupSize(&x, &y, &z);
        require(x == 64 && y == 1 && z == 1, "Unexpected shader thread-group reflection");
        D3D11_SHADER_DESC desc{};
        dxCheck(reflection->GetDesc(&desc));
        std::cout << "Verified shader reflection: threads=" << x
                  << ", bindings=" << desc.BoundResources << '\n';
    } else if (id == 44) {
        auto upload = c.buffer(256, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ);
        auto readback = c.buffer(256, D3D12_HEAP_TYPE_READBACK, D3D12_RESOURCE_STATE_COPY_DEST);
        for (uint32_t generation = 0; generation < 4; ++generation) {
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
```

### 同主题跨 API 导航

| 路线 | 程序 |
| --- | --- |
| DirectX 12 | [dx42_shader_inspection](README.md) |
| 对照后端 | [vk42_shader_inspection](../../../Vulkan/native/lessons/vk42_shader_inspection/README.md) |

### 先预测，再验证

将 shader 的 local size 改为 32，但 host 的组数不改，预测覆盖范围和验证结果。

参考推导：总调用数量减半，尾部不会按原契约写入。反射/声明、dispatch 组数、缓冲容量和 CPU 检查必须共同更新。

## 验证与边界

运行 `--smoke` 执行有限自动检查。数值实验读取实际 GPU 结果，图形实验检查非空图像并可查看读回。交互重建与性能还需额外证据；窗口 renderer 当前保留串行等待，双槽计算实验不会被描述为完整多帧呈现引擎。
