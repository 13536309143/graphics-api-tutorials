# 参考资料、阅读方式与版本差异

本轮根据你提供的两份教程，补充“概念目的 → 实际接口 → 源码追踪 → 修改实验 → 错误定位”的阅读层次。章节围绕本仓库的对象、参数和实现组织，外部资料用于查证与延伸阅读，不整页复制译文或整套代码。外部作品仍按各自许可分发。

## 你提供的入口

- [Vulkan 中文镜像：Introduction](https://docs.vulkan.net.cn/tutorial/latest/00_Introduction.html)：先读总览建立对象依赖，再读具体资源与渲染步骤。
- [BootWiki：创建一个窗口](https://www.bootwiki.com/opengl/opengl-tutorial1.html)：从窗口、状态、清屏和交换开始拆解调用。本页使用 GLUT；本仓库保留 GLFW，不把不同窗口库的初始化混接。

阅读时间：2026-11-04。`latest` 是可变化入口，以下对比记录本次读到的内容，并非永久版本承诺。

| 项目 | 中文 Vulkan 入口介绍 | 当前官方 latest 介绍 | 本仓库原生路线 |
| --- | --- | --- | --- |
| C++ / API | C++17 先决条件、原始 C API | C++20、Vulkan-Hpp RAII | C++20、原始 C API |
| 版本 | 不能仅从 URL 推断固定版本 | 介绍明确以 Vulkan 1.4 为基线 | Vulkan 1.3；nvpro 另为 1.4 |
| shader | 按具体镜像章节确认 | 介绍采用 Slang 为主要语言 | GLSL → glslc → SPIR-V |
| rendering / sync | 按具体章节确认 | dynamic rendering、timeline 等现代路线 | dynamic rendering、sync2、binary semaphore/fence |

以上摘要只用于辨认路线差异；不要根据“章节号一样”直接交换 C++ 代码。[当前官方介绍](https://docs.vulkan.org/tutorial/latest/00_Introduction.html) 和 [总览](https://docs.vulkan.org/tutorial/latest/01_Overview.html) 是核对入口。

## OpenGL 与窗口层

- [OGLdev 原作者窗口课](https://ogldev.org/www/tutorial01/tutorial01.html)：对照窗口库与 OpenGL 状态的分工。
- [GLFW window guide](https://www.glfw.org/docs/latest/window)：查询窗口、framebuffer 尺寸与缓冲交换。
- [GLFW Vulkan guide](https://www.glfw.org/docs/latest/vulkan_guide.html)：查询 loader、所需扩展和 surface 创建。

BootWiki 后续目录提供了从变换、光照到模型和高级效果的主题线索。本仓库依据已有 34 课重新组织对照，不把目录中出现的拾取、细分或运动模糊当作当前已实现功能。

## Vulkan 细节查证

- [staging buffer](https://docs.vulkan.org/tutorial/latest/04_Vertex_buffers/02_Staging_buffer.html)：比较 CPU 可见中转与最终 GPU 资源。
- [descriptor layout / buffer](https://docs.vulkan.org/tutorial/latest/05_Uniform_buffers/00_Descriptor_set_layout_and_buffer.html)：区分资源接口与内容。
- [dynamic offset](https://docs.vulkan.org/guide/latest/descriptor_dynamic_offset.html)：后续实现帧资源环时核对偏移与范围。
- [synchronization](https://docs.vulkan.org/guide/latest/synchronization.html)：按生产者和消费者选择依赖。
- [validation overview](https://docs.vulkan.org/guide/latest/validation_overview.html)：从 VUID 返回有效用法规则。

## DX12 与框架

- [Microsoft root signatures](https://learn.microsoft.com/en-us/windows/win32/direct3d12/root-signatures)：接口、根常量和描述符表。
- [Microsoft resource barriers](https://learn.microsoft.com/en-us/windows/win32/direct3d12/using-resource-barriers-to-synchronize-resource-states-in-direct3d-12)：legacy transition/UAV 等状态依赖。
- [Microsoft command lists](https://learn.microsoft.com/en-us/windows/win32/direct3d12/recording-command-lists-and-bundles)：录制对象、allocator 与执行。
- [Microsoft D3D11→12 迁移说明](https://learn.microsoft.com/en-us/windows/win32/direct3d12/porting-from-direct3d-11-to-direct3d-12)：观察显式 CPU/GPU 生命周期管理。
- [nvpro_core2 官方仓库](https://github.com/nvpro-samples/nvpro_core2)：以本仓库固定子模块提交为接口依据，避免直接套最新主分支示例。

## 如何组合使用

先从本地对照页打开真实代码，再去外部资料核对一个不理解的术语或约束。出现差异时记录版本与假设，回到本工程决定是保留当前基线还是开启迁移练习。不要为了让某个网上片段编译而同时升级 API、shader 语言和框架版本，这会让新错误难以归因。
