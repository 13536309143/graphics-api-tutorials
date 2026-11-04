# 原生 Vulkan · C++20 / CMake

[合集首页](../../README.md)

本路线有 10 个独立程序，从设备创建到直接光 PBR 和离屏传输。要求：Vulkan 1.3+；启用 validation layer 时自动计数 VUID 类错误。加载器报告的外部覆盖层/manifest 诊断会单独输出，不与应用验证错误混为一谈。

## 部署

先按合集首页安装工具并递归克隆。从仓库根目录执行：

```powershell
cmake --preset vulkan-native
cmake --build --preset vulkan-native-debug --parallel 4
ctest --preset vulkan-native-debug
```

程序位于 `Vulkan/native/build/bin/Debug`，shader 在旁边 shaders/，改源 shader 后重新 build。Release 使用 `vulkan-native-release`。每课也可在本地阅读器中点击运行。

| 程序 | 课程 |
| --- | --- |
| vk01_instance_device | [实例、设备与队列](lessons/vk01_instance_device/README.md) |
| vk02_swapchain | [Surface 与交换链](lessons/vk02_swapchain/README.md) |
| vk03_triangle | [动态渲染与第一个三角形](lessons/vk03_triangle/README.md) |
| vk04_vertex_buffer | [顶点缓冲与布局](lessons/vk04_vertex_buffer/README.md) |
| vk05_indexed_uniform | [索引、UBO 与 Push Constants](lessons/vk05_indexed_uniform/README.md) |
| vk06_texture_descriptor | [纹理、Staging 与 Descriptor](lessons/vk06_texture_descriptor/README.md) |
| vk07_depth_instancing | [深度与实例化](lessons/vk07_depth_instancing/README.md) |
| vk08_compute_readback | [Compute、屏障与 GPU 读回](lessons/vk08_compute_readback/README.md) |
| vk09_pbr | [直接光 PBR](lessons/vk09_pbr/README.md) |
| vk10_offscreen | [离屏目标与图像传输](lessons/vk10_offscreen/README.md) |

## 代码结构

每课 main.cpp 选择该课执行路径；共享初始化、资源与命令代码集中在 common/ 下。Vulkan/common/scene_vk.hpp 是原生和 nvpro 共用的低层渲染器；native/common/native_app.hpp 负责手工设备、Surface/交换链和同步。几何与 Frame/Draw 布局在 shared/scene.hpp。共享代码仍是教学源码，可直接打断点，不是黑盒二进制依赖。

## 调试与范围

参见合集 [调试指南](../../docs/DEBUGGING.md)。Esc 关闭图形窗口；设备/计算课输出日志后立即退出；`--smoke` 用固定时间执行并写 GPU 读回图像。

本路线尚未移植 OpenGL 的模型加载、完整 IBL、Bloom/SSAO、骨骼动画等所有高级效果。当前 PBR 是单次散射直接光；同步使用单帧串行等待，未实现高吞吐多帧资源环。窗口支持尺寸重建；更完整的设备丢失恢复、多窗口和跨平台适配属于后续扩展。
