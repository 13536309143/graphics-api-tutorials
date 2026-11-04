# dx33_debug_labels · GPU 调试标签

[54 课目录](../../COURSE.md)

## 目的与实际执行

debug layer 要在 Device 创建前启用，InfoQueue 检查 ERROR/CORRUPTION。实验通过 AddApplicationMessage 验证信息通道并为 queue/list/allocator SetName。原始 BeginEvent 是工具层诊断接口；课程不直接伪造 PIX 事件载荷，嵌套 PIX 标记需接入其正式 helper。

本程序有独立实验选择与验证路径，公共实现集中在 advanced_labs.cpp。设备/API 检查、数值 compute 与图形渲染采用不同证据；不是每个程序都会弹出窗口。

## 构建和运行

```powershell
cmake --preset dx12
cmake --build --preset dx12-debug --target dx33_debug_labels --parallel 4
.\DirectX12\build\bin\Debug\dx33_debug_labels.exe
```

## 深入学习：数据、接口与执行

- [第 05 课：验证层、调试队列与诊断证据](../../chapters/05_validation/README.md)

### 本课的真实源码切片

来自 [DirectX12/common/advanced_labs.cpp](../../common/advanced_labs.cpp#L809)，第 809—842 行；这是现有实现的局部摘录，末尾可能处于函数中间，不是可单独编译的完整程序。

```cpp
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
| DirectX 12 | [dx33_debug_labels](README.md) |
| 对照后端 | [vk33_debug_labels](../../../Vulkan/native/lessons/vk33_debug_labels/README.md) |

### 先预测，再验证

注入一条 INFO 应用消息，确认它会进入日志/回调而不会增加 API 错误计数。

参考推导：消息严重度与错误计数策略独立。合法的应用信息用于测试诊断渠道，不应冒充 validation error 或通过清空错误队列让检查成功。

## 验证与边界

运行 `--smoke` 执行有限自动检查。数值实验读取实际 GPU 结果，图形实验检查非空图像并可查看读回。交互重建与性能还需额外证据；窗口 renderer 当前保留串行等待，双槽计算实验不会被描述为完整多帧呈现引擎。
