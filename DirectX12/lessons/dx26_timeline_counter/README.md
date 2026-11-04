# dx26_timeline_counter · 计数同步与生产消费

[54 课目录](../../COURSE.md)

## 目的与实际执行

ID3D12Fence 本来就是计数模型，queue Signal/Wait 建立生产消费，CPU 再等待完成值。相同队列链路用于验证语义；跨队列还需合法 resource state 与访问依赖，不声称此例展示独立引擎并行加速。

本程序有独立实验选择与验证路径，公共实现集中在 advanced_labs.cpp。设备/API 检查、数值 compute 与图形渲染采用不同证据；不是每个程序都会弹出窗口。

## 构建和运行

```powershell
cmake --preset dx12
cmake --build --preset dx12-debug --target dx26_timeline_counter --parallel 4
.\DirectX12\build\bin\Debug\dx26_timeline_counter.exe
```

## 深入学习：数据、接口与执行

- [第 50 课：Timeline、Fence 计数与生产消费链](../../chapters/50_timeline/README.md)

### 本课的真实源码切片

来自 [DirectX12/common/advanced_labs.cpp](../../common/advanced_labs.cpp#L786)，第 786—829 行；这是现有实现的局部摘录，末尾可能处于函数中间，不是可单独编译的完整程序。

```cpp
        std::cout << "Verified queue producer 1 -> consumer 2 fence chain\n";
    } else if (id == 31) {
        for (int i = 0; i < 4; ++i)
            stagingCopy(c);
        std::cout << "Retired 4 UPLOAD/DEFAULT/READBACK generations after fence completion\n";
    } else if (id == 32) {
        ComPtr<ID3DBlob> cache, exactShader;
        {
            ComputeKernel first(c, options);
            first.execute(0, 257, 3, 7);
            require(readBuffer(first.readback.Get(), 257)[256] == 775,
                    "Initial cached PSO result mismatch");
            dxCheck(first.pipeline->GetCachedBlob(&cache));
            exactShader = first.compiledShader;
        }
        {
            ComputeKernel second(c, options, cache.Get(), exactShader.Get());
            second.execute(0, 257, 3, 7);
            require(readBuffer(second.readback.Get(), 257)[256] == 775,
                    "Reloaded cached PSO mismatch");
        }
        std::cout << "Verified cached PSO reload, bytes=" << cache->GetBufferSize() << '\n';
    } else if (id == 33) {
        dxCheck(c.queue->SetName(L"Course Graphics Queue"));
        dxCheck(c.cmd->SetName(L"Course Upload Commands"));
        dxCheck(c.allocator->SetName(L"Course Upload Allocator"));
        c.begin();
        c.submit();
        std::cout << "Named queue, command list and allocator for GPU debugging; event regions "
                     "require PIX helpers\n";
    } else {
        ComputeKernel kernel(c, options);
        uint32_t mode = id == 30 ? 2 : id == 14 || id == 15 ? 1 : 0;
        uint32_t count = id == 28 || id == 30 ? 257 : 128;
        for (uint32_t slot = 0; slot < (id == 15 ? 3u : 1u); ++slot) {
            uint32_t factor = id == 16 ? 9 : 3, bias = id == 16 ? 17 : 7;
            kernel.execute(mode, count, factor, bias, slot);
            auto result = readBuffer(kernel.readback.Get(), mode == 2 ? (count + 63) / 64 : count);
            if (mode == 2) {
                auto sum = std::accumulate(result.begin(), result.end(), 0u);
                require(sum == count * (count + 1) / 2, "Shared reduction result mismatch");
            } else {
                if (mode == 1) {
                    factor = slot == 0 ? 2 : slot == 1 ? 5 : 7;
```

### 同主题跨 API 导航

| 路线 | 程序 |
| --- | --- |
| DirectX 12 | [dx26_timeline_counter](README.md) |
| 对照后端 | [vk26_timeline_counter](../../../Vulkan/native/lessons/vk26_timeline_counter/README.md) |

### 先预测，再验证

说明 host 等 counter=2 与 GPU queue 等 counter=1 的区别。

参考推导：前者阻塞 CPU 到某进度，后者让 GPU 工作在依赖成立后执行，CPU 可以继续录制其他工作。等待对象相同不意味着阻塞同一执行主体。

## 验证与边界

运行 `--smoke` 执行有限自动检查。数值实验读取实际 GPU 结果，图形实验检查非空图像并可查看读回。交互重建与性能还需额外证据；窗口 renderer 当前保留串行等待，双槽计算实验不会被描述为完整多帧呈现引擎。
