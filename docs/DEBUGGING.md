# 调试：CPU、API 验证与 GPU 帧

各章现有自己的 C++、着色器、资源和 CMake，关键步骤有中文注释；可直接复制整个章节目录单独构建或打开调试。[每章独立工程与调试说明](INDEPENDENT_CHAPTERS.md)。

## Visual Studio

打开仓库文件夹，选择根 CMake configure preset（例如 vulkan-native），构建 Debug 并将 vk03_triangle 设为启动目标。CPU 断点从各课 main.cpp 进入，然后到 NativeDevice/SceneVk、NvLesson 回调或 DxDevice/DxScene。Debug 有符号，CMake 会复制运行资源。不要把 Vulkan 的 GLSL main 误当成 C++ 可直接单步的函数。

也可打开对应 build 下生成的 solution。运行参数填写 `--smoke` 可快速退出并保存读回；DX12 可加 `--warp` 做软件参考，不代表硬件性能。NV smoke 是 headless，普通启动才有 docked UI。

## VS Code

安装 Microsoft C/C++ 扩展，打开仓库根目录。已提供 .vscode/launch.json 与 tasks.json，可选 OpenGL PBR、Vulkan Native、Vulkan nvpro、DirectX 12。按 F5 会先配置/构建对应 Debug，再使用 cppvsdbg。需在包含 Windows SDK/Vulkan SDK 环境的 VS Code 会话下运行。

## Vulkan validation

SDK 提供 VK_LAYER_KHRONOS_validation。原生路线会检测是否安装；NV 路线明确启用并使用 debug utils。ERROR 级验证消息被计数，--smoke 返回非零。Vulkan loader 的第三方 overlay/manifest 错误单独记录，它们不是 shader/barrier 的 VUID；本项目不擅自修改系统注册表或关闭其他软件层。

断点优先看资源创建参数、pipeline format、descriptor binding/范围、旧新 image layout、stage/access、队列族和释放顺序。Synchronization validation 可用 Vulkan Configurator 创建应用专用配置；首次先确保普通验证零错误，不要以“画面显示”代替内存依赖正确。

[Khronos validation 概览](https://docs.vulkan.org/guide/latest/validation_overview.html) 与 [同步指南](https://docs.vulkan.org/guide/latest/synchronization.html) 是规则依据。

## DirectX 12 debug layer / DRED

代码在 CreateDevice 前尝试启用 D3D12 debug layer；未安装时提示安装 Windows 可选功能 Graphics Tools。InfoQueue 中 ERROR/CORRUPTION 会让 smoke 失败；无调试层只能验证运行，不算 API 验证通过。不要在 CreateDevice 之后才打开 debug layer。

设备移除时检查 HRESULT、GetDeviceRemovedReason，进一步在创建设备前配置 DRED breadcrumbs/page fault 诊断。当前课程未实现完整设备丢失恢复；诊断和重新创建整个 GPU 状态是不同工作。官方说明见 [D3D12 debug layer](https://learn.microsoft.com/en-us/windows/win32/direct3d12/understanding-the-d3d12-debug-layer)。

## RenderDoc / PIX / Nsight

RenderDoc：Launch Application 指向图形课 exe，工作目录选其 bin/Debug；先正常启动，再抓一帧，按 draw 查看顶点输入、descriptor/root 资源、附件和 depth。不要用只运行四帧的 smoke 抓手动帧。NV 是 offscreen→ImGui，需找到场景 draw 而非仅看最后 UI pass。[官方抓帧说明](https://renderdoc.org/docs/how/how_capture_frame.html)。

PIX：用于 Windows D3D12，Launch 指向 dx09_pbr.exe，做 GPU Capture，检查 event list、root signature、resource state 与 shader。HLSL 本课用 D3DCompiler/SM5.1，Debug 编译启用调试信息；SM6/DXC、DXR 等是后续路线，不要把特定工具的高级调试功能当成当前后端已实现。[PIX 官方入口](https://devblogs.microsoft.com/pix/)。

Nsight Graphics：用于受支持 GPU/驱动的 Vulkan/DX12 帧分析；首次先用简单三角形建立可抓取环境，再抓 PBR。工具支持矩阵会随版本变化，按官方要求安装；本项目没有要求安装 CUDA 或 Aftermath 才能运行基础课程。

## 性能前提

这些显式后端按帧等待 GPU，以单份 UBO 和目标建立清晰教学契约。不能以当前帧率比较三种 API 的性能，也不能直接删除 wait；多帧重叠必须引入每帧 allocator/command pool、常量缓冲、fence、图像/资源所有权与 upload 生命周期。先在 validation 零错误下完成资源环，再做 GPU timestamp 测量。
