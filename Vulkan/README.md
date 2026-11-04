# Vulkan 两条路线

- [原生 Vulkan](native/README.md)：10 课，手工创建实例、设备、交换链、内存、描述符、pipeline 和同步。
- [nvpro_core2](nvpro/README.md)：6 课，Context/nvapp/RenderTarget/VMA 加上同一套低层 shader/场景。

先读原生再读框架，逐项标记哪些代码被框架管理、哪些 GPU 依赖仍由自己负责。依赖版本、部署和调试说明分别在路线 README 与根 docs/ 中。
