# Vulkan / nvpro_core2 路线

[合集首页](../../README.md) · [原生路线](../native/README.md)

6 个真实使用 nvvk/nvapp/VMA/RenderTarget 的示例。与原生路线共享场景和低层管线，明确展示框架封装边界，而不是将同一个原生程序改名。

## 固定依赖与构建

nvpro_core2 固定 commit `71ab4212aa3dd8e3b6cb659ed691b59062dc5f11`，以 Git 子模块形式保存，不能只下载本仓库 ZIP 后忽略子模块。

```powershell
git submodule update --init --recursive
cmake --preset vulkan-nvpro
cmake --build --preset vulkan-nvpro-debug --parallel 4
ctest --preset vulkan-nvpro-debug
.\Vulkan\nvpro\build\bin\Debug\nv06_pbr_viewport.exe
```

以上从仓库根目录执行。依赖要求 Vulkan 1.4 的驱动和 C++20。所选框架内部要求 1.4.341 头文件，故项目下载并固定官方 Vulkan-Headers 和 Vulkan-Utility-Libraries 1.4.341，并使用 nvpro 子模块自带 Volk，避免 SDK 1.4.335 的头文件/loader 辅助源码不匹配。Vulkan loader/import library 与 glslc 仍来自本机 SDK；不修改系统安装。

只开启 nvutils/nvgui/nvapp/nvvk，未强制安装 Slang、CUDA、Aftermath 或光追扩展。自定义 shader 使用 GLSL→SPIR-V，框架内部必要的预编译 shader 由上游提供。项目包含上游许可证，应用元素结构参考了 NVIDIA project_template，见根 NOTICE。

| 程序 | 课程 |
| --- | --- |
| nv01_context | [Context 与显式资源所有权](lessons/nv01_context/README.md) |
| nv02_application | [nvapp Application 生命周期](lessons/nv02_application/README.md) |
| nv03_triangle | [动态渲染与共享场景](lessons/nv03_triangle/README.md) |
| nv04_vma_staging_texture | [VMA、Staging 与采样纹理](lessons/nv04_vma_staging_texture/README.md) |
| nv05_compute_readback | [Compute 与读回验证](lessons/nv05_compute_readback/README.md) |
| nv06_pbr_viewport | [PBR 视口与 UI 参数](lessons/nv06_pbr_viewport/README.md) |

## 生命周期和调试

Context→Application→Element attach→resize/UI/render→detach→Application deinit→Context deinit。nvvk 对象通常要求显式 deinit，不能将它当成自动 RAII。Settings/Viewports 是 ImGui UI；普通窗口 Esc 退出。

`--smoke` 使用 nvapp headless 渲染四帧并读回 offscreen，仍需要 Vulkan GPU。自定义 debug messenger 计数验证错误；不把框架内部日志输出误当成测试断言。调试请在 common/nvapp_course.hpp 的回调、共享 SceneVk 或子模块源码中设置断点，进一步使用 [RenderDoc/Nsight 指南](../../docs/DEBUGGING.md)。

此路线没有实现 ray tracing、Slang 材质编译或 glTF 框架加载，更没有把上游未使用的所有功能都宣称为课程。PBR 为直接光；UI 粗糙度控制与图像读回已实现。当前串行等待是资源生命周期教学取舍，多帧优化必须同时维护帧资源与附件所有权。


## 深入精讲与横向阅读

本路线每课已加入数据/接口/执行推导、真实源码切片和预测实验。配合[16 章三 API 精讲](../../docs/COMPARE.md)与[并排源码实验室](../../docs/compare/index.html)阅读；[功能矩阵](../../docs/FEATURE_MATRIX.md)区分已经实现与待迁移内容。


## 与原生 54 课逐课对照

按用户选择，本路线作为原生 Vulkan 的框架对照：[54 课映射](CHAPTER_MAP.md)。现有六个框架程序保留独立构建/运行；新增模型、mip、MSAA 等在原生教材中注明框架接入责任，不把未接入功能计作框架实现。
