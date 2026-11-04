# DirectX 12 · 54 课 / 54 个独立实验

[按顺序学习 54 课](COURSE.md) · [逐章跨 API 对照](../docs/CURRICULUM.md)

主线按指定官方教程顺序展开，从初始化到 compute，再加工程进阶。全部 54 个程序有实际 API/GPU 路径和独立验收；其中查询/诊断课正常输出后退出，图形课显示窗口，计算课逐项验证结果。章号是教学次序，实验编号保留旧链接。

新增静态 OBJ/glTF、图片、GPU mip、MSAA、动态常量、双槽计算资源环、timeline/fence、时间戳、管线缓存、间接绘制与 GPU 顶点生成。完整 glTF 材质/skin、IBL、SSAO 及多帧呈现仍有明确边界。

## 基础快速入口与部署

[合集首页](../README.md)

本路线有 10 个独立程序，从设备创建到直接光 PBR 和离屏传输。要求：Windows 10/11、D3D12 feature level 12_0。调试层存在时自动启用，InfoQueue 中 ERROR/CORRUPTION 会导致 smoke 失败；没有 Graphics Tools 时能运行，但不能宣称已完成调试层验证。

## 部署

先按合集首页安装工具并递归克隆。从仓库根目录执行：

```powershell
cmake --preset dx12
cmake --build --preset dx12-debug --parallel 4
ctest --preset dx12-debug
```

程序位于 `DirectX12/build/bin/Debug`，shader 在旁边 shaders/，改源 shader 后重新 build。Release 使用 `dx12-release`。每课也可在本地阅读器中点击运行。

| 程序 | 课程 |
| --- | --- |
| dx01_device | [设备、适配器与调试层](lessons/dx01_device/README.md) |
| dx02_swapchain | [交换链、RTV 与呈现](lessons/dx02_swapchain/README.md) |
| dx03_triangle | [Root Signature、PSO 与三角形](lessons/dx03_triangle/README.md) |
| dx04_vertex_buffer | [顶点缓冲与 VBV](lessons/dx04_vertex_buffer/README.md) |
| dx05_indexed_cbv | [索引、CBV 与 Root Constants](lessons/dx05_indexed_cbv/README.md) |
| dx06_texture_srv | [纹理上传、SRV 与描述符堆](lessons/dx06_texture_srv/README.md) |
| dx07_depth_instancing | [DSV、深度与实例化](lessons/dx07_depth_instancing/README.md) |
| dx08_compute_uav | [Compute UAV 与 GPU 读回](lessons/dx08_compute_uav/README.md) |
| dx09_pbr | [HLSL 直接光 PBR](lessons/dx09_pbr/README.md) |
| dx10_offscreen | [离屏 RTV 与 CopyResource](lessons/dx10_offscreen/README.md) |

## 代码结构

每课 main.cpp 选择该课执行路径；共享初始化、资源与命令代码集中在 common/ 下。common/dx12_course.hpp 包含设备、命令、资源与绘制；shaders/scene.hlsl 包含 VS/PS/CS，Debug 编译保留调试信息。几何与 Frame/Draw 布局在 shared/scene.hpp。共享代码仍是教学源码，可直接打断点，不是黑盒二进制依赖。

## 调试与范围

参见合集 [调试指南](../docs/DEBUGGING.md)。Esc 关闭图形窗口；设备/计算课输出日志后立即退出；`--smoke` 用固定时间执行并写 GPU 读回图像。

本路线尚未移植 OpenGL 的模型加载、完整 IBL、Bloom/SSAO、骨骼动画等所有高级效果。当前 PBR 是单次散射直接光；同步使用单帧串行等待，未实现高吞吐多帧资源环。窗口支持尺寸重建；更完整的设备丢失恢复、多窗口和跨平台适配属于后续扩展。


## 深入精讲与横向阅读

本路线每课已加入数据/接口/执行推导、真实源码切片和预测实验。配合[16 章三 API 精讲](../docs/COMPARE.md)与[并排源码实验室](../docs/compare/index.html)阅读；[功能矩阵](../docs/FEATURE_MATRIX.md)区分已经实现与待迁移内容。
