# 三种 API 的概念对照

| 概念 | OpenGL | Vulkan | DirectX 12 |
| --- | --- | --- | --- |
| GPU/上下文 | context + driver | Instance / PhysicalDevice / Device | DXGI Adapter / ID3D12Device |
| 图形程序与状态 | Program + 分散状态 | Graphics Pipeline + PipelineLayout | PSO + Root Signature |
| 几何输入 | VAO/VBO/EBO | Buffer + vertex input + index type | Resource + VBV/IBV + input layout |
| 常量 | uniforms / UBO | descriptor UBO / push constants | CBV / root constants |
| 纹理绑定 | texture unit | descriptor set image/sampler | SRV descriptor table / sampler |
| 目标 | default framebuffer / FBO | Swapchain images / image views | DXGI backbuffers / RTV/DSV |
| 命令 | 驱动记录即时调用 | CommandBuffer / QueueSubmit | CommandList / ExecuteCommandLists |
| 同步 | 隐式规则 + sync 对象 | stages/access/layout + semaphore/fence | resource state barriers + queue fence |
| CPU 可见内存 | map buffer | memory type + map/flush/invalidate | upload/readback heap + map |
| 计算 | compute shader（更高 GL 基线） | compute pipeline + SSBO | compute PSO + UAV |

Vulkan image layout 与 DX12 resource state 不是逐项等价枚举；它们表达合法使用和依赖的方式不同，移植时要从“谁写、谁读、在哪个队列、何时完成、何时可见”重新推导。改变一个枚举值不意味着已建立所需全部内存依赖。

本课程共享右手世界坐标、列向量与 0..1 深度投影。Vulkan 正 viewport 的 Y 映射与 D3D12 不同，因此 Vulkan 投影矩阵翻转 Y；不要同时在 shader 和矩阵里重复翻转。OpenGL 原课程保持自己的 -1..1 NDC 深度约定，不能直接拷其 projection 到另两种 API。

## 一次上传的推导

CPU 产生 checker 像素 → 写可见上传内存 → copy 到 GPU texture → transfer/copy 写入完成 → 转到 shader 采样用途并建立依赖 → 绑定 descriptor/SRV → draw。上传内存必须活到 copy 完成，绑定视图并不会替代 copy，也不会自动保护 CPU 正在改写的数据。

## 框架边界

nvpro_core2 提供 Context、VMA allocator、Application、RenderTarget 和 UI 等工具，但自定义 pass 仍有读写依赖。课程使用 GENERAL layout 的离屏目标时仍录制 color-write→fragment-read 屏障。框架减少重复劳动，不让错误的资源生命周期自动变成合法。

## 下一步

在保持场景不变的前提下，为 Vulkan/DX12 实现两帧资源环；再迁移 OpenGL 的 glTF、IBL、Bloom、SSAO 和动画。将不同后端隐藏到统一 Renderer 接口之前，先保留可独立验证的后端程序与调试视图。
