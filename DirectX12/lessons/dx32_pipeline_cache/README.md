# dx32_pipeline_cache · 管线缓存与重建

[54 课目录](../../COURSE.md)

## 目的与实际执行

queue timestamp frequency 将 query ticks 转成时间，结果经 ResolveQueryData/readback 等待后读取。缓存 PSO 重建复用完全相同的编译字节码；Debug 重编译可能引入变化，不能仅凭源码相同断言 cache 一定兼容。

本程序有独立实验选择与验证路径，公共实现集中在 advanced_labs.cpp。设备/API 检查、数值 compute 与图形渲染采用不同证据；不是每个程序都会弹出窗口。

## 构建和运行

```powershell
cmake --preset dx12
cmake --build --preset dx12-debug --target dx32_pipeline_cache --parallel 4
.\DirectX12\build\bin\Debug\dx32_pipeline_cache.exe
```

## 深入学习：数据、接口与执行

- [第 53 课：GPU 时间戳、管线缓存与测量边界](../../chapters/53_measure_cache/README.md)

### 本课的真实源码切片

来自 [DirectX12/common/advanced_labs.cpp](../../common/advanced_labs.cpp#L799)，第 799—842 行；这是现有实现的局部摘录，末尾可能处于函数中间，不是可单独编译的完整程序。

```cpp
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
                    bias = slot == 0 ? 3 : slot == 1 ? 11 : 13;
                }
                for (uint32_t i = 0; i < count; ++i)
                    require(result[i] == i * factor + bias, "Compute/root-binding mismatch");
            }
        }
        std::cout << "Verified experiment " << id << ", items=" << count
                  << ", constant stride=256\n";
    }
    c.checkMessages();
    return 0;
}
} // namespace course
```

### 同主题跨 API 导航

| 路线 | 程序 |
| --- | --- |
| DirectX 12 | [dx32_pipeline_cache](README.md) |
| 对照后端 | [vk32_pipeline_cache](../../../Vulkan/native/lessons/vk32_pipeline_cache/README.md) |

### 先预测，再验证

列出一个公平跨 API 性能实验至少需要固定的六项条件。

参考推导：工作量、分辨率、shader/材质、场景/相机、采样/呈现策略、构建配置；还应预热、重复采样并报告 CPU/GPU/整帧时间及波动。

## 验证与边界

运行 `--smoke` 执行有限自动检查。数值实验读取实际 GPU 结果，图形实验检查非空图像并可查看读回。交互重建与性能还需额外证据；窗口 renderer 当前保留串行等待，双槽计算实验不会被描述为完整多帧呈现引擎。
