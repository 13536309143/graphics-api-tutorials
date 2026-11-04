# 三种 API 的责任、数据流与源码对照

[进入 16 章精讲](COMPARE.md) · [交互式并排源码](compare/index.html) · [功能矩阵](FEATURE_MATRIX.md) · [实验协议](EXPERIMENTS.md)

三种 API 都要把输入、管线、目标、命令和依赖连接起来。主要区别是这些状态保存在何处、由谁管理内存与执行，以及哪些约束由应用显式声明。nvpro_core2 是 Vulkan 的工具框架，不改变底层规则。

| 责任 | OpenGL | Vulkan 原生 | Vulkan / nvpro_core2 | DirectX 12 |
| --- | --- | --- | --- | --- |
| 初始化 | context / GLAD | instance / physical / device / queues | Context | DXGI adapter / device / queue |
| 几何解释 | VAO / VBO / EBO | buffers / input descriptions | 相同机制，含 VMA staging 示例 | resources / VBV / IBV / input layout |
| 管线 | program + context state | pipeline + pipeline layout | 复用 SceneVk | PSO + root signature |
| 每帧数据 | uniforms / UBO | UBO descriptor | 相同 | root CBV |
| 每 draw 数据 | uniforms | push constants | 相同 | root constants |
| 纹理访问 | texture unit / sampler | image view / sampler / descriptor | 复用公共纹理路径 | SRV table / static sampler |
| 命令 | 驱动组织 | command pool / buffer / queue submit | Application 加自定义 pass | allocator / list / ExecuteCommandLists |
| 依赖 | 按 API 操作规则 | stage / access / layout + sync objects | 框架不替代自定义依赖 | legacy state barriers + fences |
| 输出 | default framebuffer / FBO | swapchain / image views | RenderTarget / ImGui | DXGI back buffers / RTV / DSV |

## 一份数据的完整旅程

```text
CPU 场景数据 -> 上传内存 -> GPU 资源 -> view/descriptor/输入布局
                                       -> pipeline + draw -> 目标图像
                                       -> 下一 pass / 呈现 / CPU 读回
```

描述符保存访问引用，不复制图像；创建 view 不会上传像素；录制 draw 不代表执行结束；map 不代表 GPU 已停止读取。每条箭头都应能对应到代码和明确的完成/可见性条件。

## 不能机械一一替换的地方

Vulkan image layout 与 DX12 resource state 不是逐项同义枚举。二者的依赖表达不同，应从生产者、消费者、队列和资源生命周期重新推导。GENERAL 也仍可能需要 barrier。OpenGL 的隐式安排不代表零成本，或任何 shader 写后读都无需专门考虑。

OpenGL 原课程使用 -1..1 NDC 深度；共享 Vulkan/DX12 使用 0..1、右手世界和列向量。Vulkan 在当前正 viewport 路径翻转投影 Y。DX12 不强制左手，矩阵存储与乘法记法也不能混为一谈。

## 学会比较的三个层次

概念对比解释为何需要这些对象。实现对比定位本工程真实的创建、更新、绑定与释放。实验对比固定输入并收集日志、资源和画面证据。前两层能帮助迁移代码，性能结论还需要功能对齐后的统一测量；当前串行教学后端不是 benchmark。

各主题的完整步骤、源码切片、修改练习和答案从精讲路线进入。尚未完成的后端功能在功能矩阵中标明，避免用理论支持代替实现证据。
