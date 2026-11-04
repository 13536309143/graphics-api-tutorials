# vk01_instance_device · 实例、设备与队列

[路线首页](../../README.md) · [上一课](../../README.md) · [下一课](../vk02_swapchain/README.md)

## 构建和运行

从仓库根目录执行，完成配置后后续只需 build：

```powershell
cmake --preset vulkan-native
cmake --build --preset vulkan-native-debug --target vk01_instance_device --parallel 4
.\Vulkan\native\build\bin\Debug\vk01_instance_device.exe
```

也可以通过根目录“开始学习.cmd”打开页面后点击“运行本课”。01/08 是控制台计算/设备检查示例，点击后可用“查看运行输出”阅读结果。其余示例 Esc 退出，拖动窗口可测试尺寸重建。

预期结果：只输出显卡信息；没有窗口，这是设备初始化课。

## 先读哪些代码

NativeDevice：instance → validation messenger → physical device/features → queue family → logical device。

本目录 [main.cpp](main.cpp) 选择本课的执行路径。完整 API 实现在 `common/native_app.hpp 与 ../common/scene_vk.hpp`（相对路线根目录），数学和网格在仓库 shared/scene.hpp。每课都是独立程序；共享代码避免把相同资源生命周期错误复制十遍，也会为后续课预先初始化部分公共状态。首次阅读只沿本课列出的调用路径展开。

## 原理和实现

Vulkan 区分实例、物理设备和逻辑设备。实例负责与驱动体系交互，物理设备代表适配器，逻辑设备启用选定特性并提供队列。本课要求 Vulkan 1.3、dynamicRendering、synchronization2，并选择同时支持 graphics/compute 的队列族；窗口课程还检查 present。SDK 版本不等于驱动能提供的 API 版本，必须查询设备。

## 修改练习

枚举多个设备并打印队列族；让选择策略偏向独立显卡。

修改 shader 后重新构建，再运行。Vulkan GLSL 会被编译并复制到 exe 旁的 shaders；DX12 的 HLSL 副本也随构建更新，运行时再编译。应修改源目录而不是运行副本。

## 自测

为何队列句柄必须来自创建逻辑设备时申请的队列族？

参考：逻辑设备只创建所申请的队列，不能随意拿另一个族的索引使用。

跨 API 的问题要结合本课 API 的实际对象名称回答；上面题目围绕通用概念，具体 Vulkan 名称可对照 DX12 的 CBV/SRV/UAV、root signature 和 resource barriers。

## 验证和调试

运行 `vk01_instance_device.exe --smoke` 执行四帧（或设备/计算检查）并退出；颜色课会从 GPU 读回 PPM，计算课验证 256 个数。参见仓库 [调试指南](../../../../docs/DEBUGGING.md)。这些基础例子按帧串行等待，重点是可追踪的生命周期；多帧重叠不能仅删除 wait，需要同时改成帧资源环。
