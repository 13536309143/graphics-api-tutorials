# dx37_instance_extensions · 加载器/工厂与枚举

[54 课目录](../../COURSE.md)

## 目的与实际执行

Windows SDK 提供头文件/导入库；本课程 HLSL 使用 D3DCompiler/SM5.1，未切换 DXC/SM6。DXGI 枚举 adapter 能看到硬件与软件实现；运行时 feature query 才是功能选择依据，不能只看 Visual Studio 是否识别接口名称。

CreateDXGIFactory2 的调试 flag 与 D3D12 debug layer 是相关但不同的设置。factory 枚举 adapter 并创建窗口交换链，ID3D12Device 则用于具体 GPU 对象。选择硬件前先排除或明确允许软件 adapter。

本程序有独立实验选择与验证路径，公共实现集中在 advanced_labs.cpp。设备/API 检查、数值 compute 与图形渲染采用不同证据；不是每个程序都会弹出窗口。

## 构建和运行

```powershell
cmake --preset dx12
cmake --build --preset dx12-debug --target dx37_instance_extensions --parallel 4
.\DirectX12\build\bin\Debug\dx37_instance_extensions.exe
```

## 深入学习：数据、接口与执行

- [第 02 课：工具链、SDK、依赖与可复现构建](../../chapters/02_environment/README.md)
- [第 04 课：实例扩展与 DXGI 工厂](../../chapters/04_instance_factory/README.md)

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
| DirectX 12 | [dx37_instance_extensions](README.md) |
| 对照后端 | [vk37_instance_extensions](../../../Vulkan/native/lessons/vk37_instance_extensions/README.md) |

### 先预测，再验证

故意在副本中改变运行工作目录，再从命令行启动程序，说明为何它仍应能找到 shader/assets。

参考推导：资源以 executable 所在目录为起点，构建复制到旁边。若只有 IDE 中能运行，检查是否偷偷依赖了工作目录或本机绝对路径。

把某个窗口相关要求从无窗口计算路径中移除，先说明哪些操作因此不再允许，再解释为什么计算仍能成立。

参考推导：没有 surface/swapchain 便不能走窗口呈现，但 compute pipeline、buffer 和 queue submit 不依赖可见窗口。需求应与实际功能一致。

## 验证与边界

运行 `--smoke` 执行有限自动检查。数值实验读取实际 GPU 结果，图形实验检查非空图像并可查看读回。交互重建与性能还需额外证据；窗口 renderer 当前保留串行等待，双槽计算实验不会被描述为完整多帧呈现引擎。
