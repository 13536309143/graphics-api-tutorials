# dx40_surface_support · 窗口呈现能力

[54 课目录](../../COURSE.md)

## 目的与实际执行

通过 glfwGetWin32Window 取得 HWND，再创建 flip-discard 交换链。实验检查 buffer count 与 swap effect，并实际清屏/呈现一帧。窗口句柄不是 D3D12 resource，也不保存图像内容。

课程选择两个 RGBA8 flip-discard back buffer，GetCurrentBackBufferIndex 决定本帧目标。显示同步与 tearing 的支持/flag 要另行检查，不能仅把 Present 参数设为零就称作公平性能测试。

ResizeBuffers 前释放所有 back buffer 引用并等待相关 GPU 工作；随后重建 RTV/DSV、depth 与 MSAA/离屏目标。只释放一个局部 ComPtr 而仍有其他引用，不满足重建要求。

本程序有独立实验选择与验证路径，公共实现集中在 advanced_labs.cpp。设备/API 检查、数值 compute 与图形渲染采用不同证据；不是每个程序都会弹出窗口。

## 构建和运行

```powershell
cmake --preset dx12
cmake --build --preset dx12-debug --target dx40_surface_support --parallel 4
.\DirectX12\build\bin\Debug\dx40_surface_support.exe
```

## 深入学习：数据、接口与执行

- [第 09 课：窗口 Surface 与 HWND 连接](../../chapters/09_surface/README.md)
- [第 10 课：交换链选择、格式与呈现模式](../../chapters/10_swapchain/README.md)
- [第 22 课：窗口变化与交换链重建](../../chapters/22_swapchain_recreation/README.md)

### 本课的真实源码切片

来自 [DirectX12/common/advanced_labs.cpp](../../common/advanced_labs.cpp#L592)，第 592—635 行；这是现有实现的局部摘录，末尾可能处于函数中间，不是可单独编译的完整程序。

```cpp
void surfaceInspection(const LabOptions& options) {
    require(glfwInit() != 0, "GLFW init failed");
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    auto* window = glfwCreateWindow(960, 640, "DXGI inspection", nullptr, nullptr);
    require(window != nullptr, "Window failed");
    {
        DxWindow app(window, 2, options.warp);
        DXGI_SWAP_CHAIN_DESC1 desc{};
        dxCheck(app.swap->GetDesc1(&desc));
        require(desc.BufferCount == 2 && desc.SwapEffect == DXGI_SWAP_EFFECT_FLIP_DISCARD,
                "Unexpected swapchain configuration");
        app.render(1, nullptr);
        std::cout << "Verified HWND swapchain, buffers=" << desc.BufferCount
                  << ", index=" << app.swap->GetCurrentBackBufferIndex() << '\n';
    }
    glfwDestroyWindow(window);
    glfwTerminate();
}
// 查询、诊断与字节检查有明确的结果；合法 INFO 消息可能被默认过滤，临时空过滤器只扩大接收范围。
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
```

### 同主题跨 API 导航

| 路线 | 程序 |
| --- | --- |
| DirectX 12 | [dx40_surface_support](README.md) |
| 对照后端 | [vk40_surface_support](../../../Vulkan/native/lessons/vk40_surface_support/README.md) |

### 先预测，再验证

把同一段 buffer compute 放到没有窗口的程序里，说明它与呈现路径共享哪些对象。

参考推导：可以共享 device、queue、buffer、pipeline 和命令/同步原则；surface、swapchain 与平台输入不是必需条件。

解释为何两帧 CPU 资源槽与三张交换链图像可以同时存在。

参考推导：前者跟踪应用可变资源的在途使用期，后者由 acquire/呈现系统选择。两个索引各有复用条件，不能靠数值恰好相等统一。

普通交互模式下反复拖动、最小化、恢复窗口，记录会触发哪些重建路径。

参考推导：应避免零尺寸分配/除零和旧图像引用。已有 smoke 检查有限帧输出，反复 resize 属于额外手动验收，不能混写成已自动覆盖所有组合。

## 验证与边界

运行 `--smoke` 执行有限自动检查。数值实验读取实际 GPU 结果，图形实验检查非空图像并可查看读回。交互重建与性能还需额外证据；窗口 renderer 当前保留串行等待，双槽计算实验不会被描述为完整多帧呈现引擎。
