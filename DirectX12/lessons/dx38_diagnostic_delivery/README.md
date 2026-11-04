# dx38_diagnostic_delivery · 诊断消息链路

[54 课目录](../../COURSE.md)

## 目的与实际执行

debug layer 要在 Device 创建前启用，InfoQueue 检查 ERROR/CORRUPTION。实验通过 AddApplicationMessage 验证信息通道并为 queue/list/allocator SetName。原始 BeginEvent 是工具层诊断接口；课程不直接伪造 PIX 事件载荷，嵌套 PIX 标记需接入其正式 helper。

本程序有独立实验选择与验证路径，公共实现集中在 advanced_labs.cpp。设备/API 检查、数值 compute 与图形渲染采用不同证据；不是每个程序都会弹出窗口。

## 构建和运行

```powershell
cmake --preset dx12
cmake --build --preset dx12-debug --target dx38_diagnostic_delivery --parallel 4
.\DirectX12\build\bin\Debug\dx38_diagnostic_delivery.exe
```

## 深入学习：数据、接口与执行

- [第 05 课：验证层、调试队列与诊断证据](../../chapters/05_validation/README.md)

### 本课的真实源码切片

来自 [DirectX12/common/advanced_labs.cpp](../../common/advanced_labs.cpp#L612)，第 612—655 行；这是现有实现的局部摘录，末尾可能处于函数中间，不是可单独编译的完整程序。

```cpp
void inspection(DxDevice& c, const LabOptions& options, int id) {
    if (id == 37) {
        UINT count = 0;
        for (UINT i = 0;; ++i) {
            ComPtr<IDXGIAdapter1> adapter;
            auto hr = c.factory->EnumAdapters1(i, &adapter);
            if (hr == DXGI_ERROR_NOT_FOUND)
                break;
            dxCheck(hr);
            DXGI_ADAPTER_DESC1 d{};
            dxCheck(adapter->GetDesc1(&d));
            std::wcout << L"adapter=" << d.Description << L", bytes=" << d.DedicatedVideoMemory
                       << L'\n';
            ++count;
        }
        require(count > 0, "No DXGI adapters enumerated");
    } else if (id == 38) {
        require(c.messages.Get() != nullptr, "D3D12 InfoQueue unavailable");
        dxCheck(c.messages->PushEmptyStorageFilter());
        UINT64 before = c.messages->GetNumStoredMessages();
        dxCheck(c.messages->AddApplicationMessage(D3D12_MESSAGE_SEVERITY_INFO,
                                                  "Course informational diagnostic"));
        require(c.messages->GetNumStoredMessages() > before,
                "InfoQueue did not receive application message");
        c.messages->PopStorageFilter();
        std::cout
            << "Verified informational diagnostic delivery without introducing invalid API usage\n";
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
```

### 同主题跨 API 导航

| 路线 | 程序 |
| --- | --- |
| DirectX 12 | [dx38_diagnostic_delivery](README.md) |
| 对照后端 | [vk38_diagnostic_delivery](../../../Vulkan/native/lessons/vk38_diagnostic_delivery/README.md) |

### 先预测，再验证

注入一条 INFO 应用消息，确认它会进入日志/回调而不会增加 API 错误计数。

参考推导：消息严重度与错误计数策略独立。合法的应用信息用于测试诊断渠道，不应冒充 validation error 或通过清空错误队列让检查成功。

## 验证与边界

运行 `--smoke` 执行有限自动检查。数值实验读取实际 GPU 结果，图形实验检查非空图像并可查看读回。交互重建与性能还需额外证据；窗口 renderer 当前保留串行等待，双槽计算实验不会被描述为完整多帧呈现引擎。
