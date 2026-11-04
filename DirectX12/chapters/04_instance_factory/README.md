# 第 04 课 · 实例扩展与 DXGI 工厂 · DirectX 12

[54 课目录](../../COURSE.md) · [对照精讲](../../../docs/compare/01_context_device.md) · [上一课](../03_lifetime/README.md) · [下一课](../05_validation/README.md)

## 本课目标与先修

学习目标：解释“实例扩展与 DXGI 工厂”解决的问题，找到实际对象和参数，运行对应实验并用数据/画面证明。先完成前一课，并能说明它交付的对象与完成条件；本课在该依赖上增加新契约。

## 为什么需要这一层

启动对象需要把应用需求与实现能力连接起来。先枚举再选择，可以解释失败原因；把网上扩展列表整体复制过来，会引入无关平台需求并降低可移植性。需求应来自实际使用路径。

## DirectX 12 的对象、参数与执行推导

CreateDXGIFactory2 的调试 flag 与 D3D12 debug layer 是相关但不同的设置。factory 枚举 adapter 并创建窗口交换链，ID3D12Device 则用于具体 GPU 对象。选择硬件前先排除或明确允许软件 adapter。

## 对照 Vulkan 原生：相同问题的另一种分工

实例扩展用于窗口系统连接、debug utils 等实例层功能，设备扩展在创建设备时选择。glfwGetRequiredInstanceExtensions 返回当前平台 surface 所需扩展；无窗口计算不需要同一组 WSI 条件。pApplicationInfo 中的 apiVersion 不是“申请升级显卡”。

## 关键参数与可检查不变量

| 检查项 | Vulkan 表达 | DX12 表达 | 不变量 / 证据 |
| --- | --- | --- | --- |
| 实例层需求 | instanceExtensions | DXGI factory flags | 要求与平台和实际功能匹配 |
| 设备层需求 | deviceExtensions / feature chain | feature level / feature queries | 不可把扩展名放错层次 |
| 无窗口路径 | 不需要 surface/swapchain | 不需要 HWND/swapchain | compute 依赖设备，不依赖呈现 |

先为每项指出源码中的创建/绑定位置，以及证明它成立的实际日志、字节或图像。数值中的设备限制以查询为准；固定实验尺寸不当作所有设备的上限。

## nvpro_core2 框架对照

ContextInitInfo 的 instanceExtensions 与 deviceExtensions 分开填写；框架自动创建不代表应用可以忽略扩展所属层次。

这部分是原生 Vulkan 各课的框架对照，框架可运行入口仍为原有六课。如果本节需要新接入工作，会按正文说明；不把原生实验当作已完成的框架实现。

## 运行与实际检查

| 路线 | 实际实验 |
| --- | --- |
| DirectX 12 | [dx37_instance_extensions](../../lessons/dx37_instance_extensions/README.md) |
| Vulkan 原生 | [vk37_instance_extensions](../../../Vulkan/native/lessons/vk37_instance_extensions/README.md) |

从合集根目录执行；普通图形程序 Esc 退出，设备/计算/诊断程序输出结果后结束。从“开始学习.cmd”进入本页时，表格可直接点击运行。

```powershell
cmake --preset dx12
cmake --build --preset dx12-debug --target dx37_instance_extensions --parallel 4
.\DirectX12\build\bin\Debug\dx37_instance_extensions.exe
ctest --preset dx12-debug -R "^dx37_instance_extensions$" --output-on-failure
```

检查三个层次：创建/绑定/提交的接口约束；数据读回或目标画面；资源使用结束后的回收。诊断实验用合法信息或查询证明渠道可用，不故意保留 API 错误。图形 smoke 的像素检查与数值实验的逐项检查不同，具体实现见源码。

## 关键实现：从当前源码定位

来自 [DirectX12/common/advanced_labs.cpp](../../common/advanced_labs.cpp#L612)，第 612—655 行。下列是当前仓库的局部切片，可能在函数中间结束；完整上下文请打开文件，切片不作为单独可编译程序。

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

阅读时按“前置对象 → 参数/布局 → 录制/调用 → 完成/可见性 → 释放”五步标注。共享 helper 减少重复代码，但不会取消每课的 API 契约。

## 受控修改实验

把某个窗口相关要求从无窗口计算路径中移除，先说明哪些操作因此不再允许，再解释为什么计算仍能成立。

先记录预测，再只改一个因素；保留基线与修改后的输出。不要同时改变 shader、资源范围和同步，这样即使结果变好也无法归因。

### 参考推导与通过标准

没有 surface/swapchain 便不能走窗口呈现，但 compute pipeline、buffer 和 queue submit 不依赖可见窗口。需求应与实际功能一致。

## 常见失败与排查顺序

扩展名拼写正确不代表当前实现支持；实例与设备扩展放错创建位置也不会因为同为字符串就生效。

先看第一条错误和当前执行路径，再看实际数据/绑定，最后看 shader 与画面。需要 GPU 证据时抓取正常交互帧；仅四帧的 smoke 适合自动检查，不适合手动抓帧。

## 自测与本课交付

能否不看源码回答：把某个窗口相关要求从无窗口计算路径中移除，先说明哪些操作因此不再允许，再解释为什么计算仍能成立。

提交一份简短记录：输入与假设、唯一修改、预期、实际证据、原因、是否恢复基线。如果只能复述函数名而说不清何时允许访问/释放，先回到本课的数据流。

## 官方阅读位置与当前工程差异

[指定 Vulkan 教程对应位置](https://docs.vulkan.net.cn/tutorial/latest/03_Drawing_a_triangle/00_Setup/01_Instance.html)。DX12 路线按同一概念顺序建立映射，其对象与约束以 Microsoft 文档和本后端代码为准。49—54 课包含在基础/compute 之后加入的工程扩展。

[版本与资料说明](../../../docs/REFERENCES.md) · [调试](../../../docs/DEBUGGING.md) · [功能完成度](../../../docs/FEATURE_MATRIX.md)
