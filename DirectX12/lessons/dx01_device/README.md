# dx01_device · 设备、适配器与调试层

[路线首页](../../README.md) · [上一课](../../README.md) · [下一课](../dx02_swapchain/README.md)

## 构建和运行

从仓库根目录执行，完成配置后后续只需 build：

```powershell
cmake --preset dx12
cmake --build --preset dx12-debug --target dx01_device --parallel 4
.\DirectX12\build\bin\Debug\dx01_device.exe
```

也可以通过根目录“开始学习.cmd”打开页面后点击“运行本课”。01/08 是控制台计算/设备检查示例，点击后可用“查看运行输出”阅读结果。其余示例 Esc 退出，拖动窗口可测试尺寸重建。

预期结果：只输出显卡信息；没有窗口，这是设备初始化课。

## 先读哪些代码

DxDevice / DXGI Adapter / CreateDevice

本目录 [main.cpp](main.cpp) 选择本课的执行路径。完整 API 实现在 `common/dx12_course.hpp`（相对路线根目录），数学和网格在仓库 shared/scene.hpp。每课都是独立程序；共享代码避免把相同资源生命周期错误复制十遍，也会为后续课预先初始化部分公共状态。首次阅读只沿本课列出的调用路径展开。

## 原理和实现

DXGI 枚举适配器并跳过软件设备；--warp 可显式使用软件参考设备。D3D12GetDebugInterface 成功时在创建设备前启用调试层，否则仍可渲染并提示安装 Graphics Tools。设备、队列、allocator、command list 与 fence 各有不同职责。

本课使用与 Vulkan 相同的几何和验证目标，比较资源绑定与同步的不同表达。CPU 创建的数据、GPU 资源和用于访问它的 view/descriptor 是不同层次；用调试层与抓帧工具分别核对接口约定和实际内容。命令异步执行，示例显式等待 fence 后才重用单份上传资源与 command allocator。

HLSL 使用 column_major 与 mul(M,v)，保持与共享 GLM 数据相同的内存和乘法约定。调整窗口大小时必须更新尺寸相关的目标与投影；map、创建视图或改变 CPU 变量都不会替代已经提交给 GPU 的工作。

## 修改练习

将 --warp 与硬件路径分别运行，比较适配器名称；查询 feature level。

修改 shader 后重新构建，再运行。Vulkan GLSL 会被编译并复制到 exe 旁的 shaders；DX12 的 HLSL 副本也随构建更新，运行时再编译。应修改源目录而不是运行副本。

## 自测

为什么需要在 CreateDevice 前启用调试层？

参考：设备创建后再启用可能导致设备移除；调试接口应先参与设备初始化。


## 验证和调试

运行 `dx01_device.exe --smoke` 执行四帧（或设备/计算检查）并退出；颜色课会从 GPU 读回 PPM，计算课验证 256 个数。参见仓库 [调试指南](../../../docs/DEBUGGING.md)。这些基础例子按帧串行等待，重点是可追踪的生命周期；多帧重叠不能仅删除 wait，需要同时改成帧资源环。


## 深入学习：数据、接口与执行

debug layer 必须在 CreateDevice 前启用，InfoQueue 后续收集使用错误。选择 adapter 与请求 feature level 是能力筛选，不代表自动启用了所有可选新功能。

配套精讲：[三 API 对照 · 窗口、设备与队列](../../../docs/compare/01_context_device.md)。先读概念与参数选择，再返回本课实现核对，不必一次展开所有公共类。

### 本课的真实源码切片

来自 [DirectX12/common/dx12_course.hpp](../../common/dx12_course.hpp#L71)，第 71—100 行；这是现有实现的局部摘录，末尾可能处于函数中间，不是可单独编译的完整程序。

```cpp
    explicit DxDevice(bool warp = false) {
        UINT flags = 0;
        ComPtr<ID3D12Debug> debug;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug)))) {
            debug->EnableDebugLayer();
            flags |= DXGI_CREATE_FACTORY_DEBUG;
            std::cout << "D3D12 debug layer enabled\n";
        } else
            std::cout
                << "D3D12 debug layer unavailable; install Windows Graphics Tools for validation\n";
        dxCheck(CreateDXGIFactory2(flags, IID_PPV_ARGS(&factory)));
        ComPtr<IDXGIAdapter1> adapter;
        if (warp) {
            dxCheck(factory->EnumWarpAdapter(IID_PPV_ARGS(&adapter)));
            dxCheck(
                D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&device)));
        } else {
            for (UINT i = 0; factory->EnumAdapterByGpuPreference(
                                 i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&adapter)) !=
                             DXGI_ERROR_NOT_FOUND;
                 ++i) {
                DXGI_ADAPTER_DESC1 desc;
                adapter->GetDesc1(&desc);
                if (!(desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) &&
                    SUCCEEDED(D3D12CreateDevice(
                        adapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&device))))
                    break;
                adapter.Reset();
            }
            if (!device)
```

### 同主题跨 API 导航

以下是同主题参考；场景、功能完成度和版本基线可能不同，具体边界见对照章节。

| 路线 | 可运行参照 / 当前范围 |
| --- | --- |
| OpenGL | [01_window](../../../OpenGL/lessons/01_window/README.md) |
| Vulkan 原生 | [vk01_instance_device](../../../Vulkan/native/lessons/vk01_instance_device/README.md) |
| Vulkan / nvpro_core2 | [nv01_context](../../../Vulkan/nvpro/lessons/nv01_context/README.md) |
| DirectX 12 | [dx01_device](README.md) |

### 先预测，再验证

--warp 的结果适合作为什么参考？

参考推导：适合软件正确性和排错，不适合代表真实显卡性能，也不替代硬件驱动上的验证。

在一次实验中只改一个因素，记录预测、实际画面/日志和原因。检查已有 smoke 是否仍通过；图形调试器里再核对本次 draw 的实际输入/输出，不能仅凭进程正常退出下结论。
