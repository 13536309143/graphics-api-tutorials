# vk02_swapchain · Surface 与交换链

[路线首页](../../README.md) · [上一课](../vk01_instance_device/README.md) · [下一课](../vk03_triangle/README.md)

## 构建和运行

从仓库根目录执行，完成配置后后续只需 build：

```powershell
cmake --preset vulkan-native
cmake --build --preset vulkan-native-debug --target vk02_swapchain --parallel 4
.\Vulkan\native\build\bin\Debug\vk02_swapchain.exe
```

也可以通过根目录“开始学习.cmd”打开页面后点击“运行本课”。01/08 是控制台计算/设备检查示例，点击后可用“查看运行输出”阅读结果。其余示例 Esc 退出，拖动窗口可测试尺寸重建。

预期结果：深蓝清屏窗口，支持缩放和最小化恢复。

## 先读哪些代码

NativeDevice surface → Swapchain::recreate → nativeMain acquire/submit/present。

本目录 [main.cpp](main.cpp) 选择本课的执行路径。完整 API 实现在 `common/native_app.hpp 与 ../common/scene_vk.hpp`（相对路线根目录），数学和网格在仓库 shared/scene.hpp。每课都是独立程序；共享代码避免把相同资源生命周期错误复制十遍，也会为后续课预先初始化部分公共状态。首次阅读只沿本课列出的调用路径展开。

## 原理和实现

GLFW 创建无 OpenGL 上下文的窗口，Surface 是平台窗口和 Vulkan 的桥梁。交换链负责可呈现图像，实际尺寸/格式/数量受 SurfaceCapabilities 约束。Acquire 的 semaphore 让渲染等待图像可用；提交后另一个 semaphore 让 present 等待绘制完成。缩放遇到 OUT_OF_DATE 或 SUBOPTIMAL 需要重建，而最小化时不能创建零尺寸附件。

## 修改练习

改变清屏颜色；在调整窗口大小时记录交换链 extent 与图像数。

修改 shader 后重新构建，再运行。Vulkan GLSL 会被编译并复制到 exe 旁的 shaders；DX12 的 HLSL 副本也随构建更新，运行时再编译。应修改源目录而不是运行副本。

## 自测

Acquire 成功之后是否可以立刻让 present 使用未提交的图像内容？

参考：不可以，需要提交正确命令并用 semaphore 建立渲染完成到 present 的依赖。

跨 API 的问题要结合本课 API 的实际对象名称回答；上面题目围绕通用概念，具体 Vulkan 名称可对照 DX12 的 CBV/SRV/UAV、root signature 和 resource barriers。

## 验证和调试

运行 `vk02_swapchain.exe --smoke` 执行四帧（或设备/计算检查）并退出；颜色课会从 GPU 读回 PPM，计算课验证 256 个数。参见仓库 [调试指南](../../../../docs/DEBUGGING.md)。这些基础例子按帧串行等待，重点是可追踪的生命周期；多帧重叠不能仅删除 wait，需要同时改成帧资源环。


## 实测画面

固定时间的 GPU 读回图像（nvpro 为 headless Viewport 输出）。

![vk02_swapchain](../../../../docs/previews/vk02_swapchain.png)
