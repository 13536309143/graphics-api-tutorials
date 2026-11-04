# 三种 API 精讲与对照路线

[打开交互式对照实验室](compare/index.html) · [功能完成度](FEATURE_MATRIX.md) · [统一实验协议](EXPERIMENTS.md) · [参考资料与版本差异](REFERENCES.md)

这套路线围绕同一个渲染问题解释 OpenGL、Vulkan 和 DirectX 12。Vulkan 另列原生与 nvpro_core2 两条实现方式。每章包含目的、对象关系、参数选择、真实源码、修改实验、错误定位和自测推导；不是把函数名换成三列就算完成对照。

## 16 个精讲主题

| 主题 | 要解决的问题 |
| --- | --- |
| [学习路线与版本基线](compare/00_learning_path.md) | 换 API 后，哪些图形学概念没有改变？ |
| [窗口、设备与队列](compare/01_context_device.md) | 窗口、context、surface、device 分别拥有什么？ |
| [三角形、管线与着色器](compare/02_triangle_pipeline.md) | shader 成功编译后，为何仍可能黑屏？ |
| [顶点、索引与内存](compare/03_geometry_memory.md) | 为什么立方体可以有 24 个顶点而非 8 个？ |
| [矩阵、相机与坐标](compare/04_transforms_camera.md) | 为什么不能只加一次 transpose 来修复所有移植问题？ |
| [常量与资源绑定](compare/05_binding_constants.md) | root parameter 下标为什么可以不同于 HLSL register？ |
| [纹理、Mip 与颜色空间](compare/06_textures_color.md) | 过滤模式能不能修复错误的 row pitch？ |
| [目标、深度与 MSAA](compare/07_targets_depth_msaa.md) | 为什么绑定新目标后还必须检查 viewport？ |
| [同步与可见性](compare/08_synchronization.md) | GENERAL 布局为什么仍可能需要 barrier？ |
| [命令、帧槽与多帧设计](compare/09_commands_frames.md) | 为什么 frameSlot 与 imageIndex 不是同一个索引？ |
| [计算与读回](compare/10_compute_readback.md) | 256 改为 257 时需要同时改哪四处？ |
| [模型、节点与骨骼迁移](compare/11_models_animation.md) | 为什么绑定姿态矩阵与当前骨骼矩阵不能混为一谈？ |
| [直接光 PBR 与 IBL](compare/12_lighting_pbr_ibl.md) | 四张 PBR 截图为什么不能直接当逐像素回归？ |
| [阴影、SSAO 与后处理](compare/13_multipass_effects.md) | 为什么只迁移一张最终图而不看中间附件不可靠？ |
| [调试、抓帧与公平测量](compare/14_debug_performance.md) | 为什么 Debug FPS 无法公平排序三个 API？ |
| [框架与多后端架构](compare/15_framework_architecture.md) | 移除 nvpro 后哪些部分可以保持不变？ |

## 怎样使用

首次学习时从窗口和三角形开始，已有 OpenGL 基础则按常量、纹理和同步逐章横向阅读。对照实验室可以按主题并排显示真实源码，也可只显示一条路线；各示例旁有阅读与运行入口。点击运行仍需从根目录“开始学习.cmd”进入，本地 HTML 和 GitHub Markdown 都可独立阅读。

每个课程 README 也已补上针对性的深入说明、真实源码切片、同主题导航和预测实验。OpenGL 的旧独立构建命令统一调整为合集根 preset，避免误构建总启动器目录。

## 先区分实现与设计

本次深化既有 60 个可执行课程，没有把理论迁移章节计作新程序。直接光 PBR、基础纹理/深度/离屏有实际后端；Vulkan/DX12 的模型、IBL、SSAO 等仍标为迁移设计。源码切片只摘当前实现的一段，可能在函数中间结束，完整上下文由文件链接打开。

同主题导航并不承诺画面、UI 和性能完全一致。想做严格实验先固定共享输入、输出和计时条件，参照统一实验协议。不要用 Debug smoke 的执行秒数或现有窗口 FPS 给 API 排名。
