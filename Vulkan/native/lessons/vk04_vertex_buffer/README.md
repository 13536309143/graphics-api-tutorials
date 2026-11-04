# vk04_vertex_buffer · 顶点缓冲与布局

[路线首页](../../README.md) · [上一课](../vk03_triangle/README.md) · [下一课](../vk05_indexed_uniform/README.md)

## 构建和运行

从仓库根目录执行，完成配置后后续只需 build：

```powershell
cmake --preset vulkan-native
cmake --build --preset vulkan-native-debug --target vk04_vertex_buffer --parallel 4
.\Vulkan\native\build\bin\Debug\vk04_vertex_buffer.exe
```

也可以通过根目录“开始学习.cmd”打开页面后点击“运行本课”。01/08 是控制台计算/设备检查示例，点击后可用“查看运行输出”阅读结果。其余示例 Esc 退出，拖动窗口可测试尺寸重建。

预期结果：彩色矩形，由六个非索引顶点组成。

## 先读哪些代码

shared/scene.hpp::geometry(4) → VkBufferResource::init/upload → vertex attributes/stride → vkCmdBindVertexBuffers。

本目录 [main.cpp](main.cpp) 选择本课的执行路径。完整 API 实现在 `common/native_app.hpp 与 ../common/scene_vk.hpp`（相对路线根目录），数学和网格在仓库 shared/scene.hpp。每课都是独立程序；共享代码避免把相同资源生命周期错误复制十遍，也会为后续课预先初始化部分公共状态。首次阅读只沿本课列出的调用路径展开。

## 原理和实现

顶点布局必须在 CPU 结构体、pipeline 的 binding/attribute 和 shader location 之间一致。本课 Vertex 为 position/normal(or color)/uv，共 32 字节，三个 offset 是 0/12/24。为了看清流程，原生入门直接使用 host-visible coherent 缓冲上传几何；这不代表离散 GPU 的最佳布局。Vulkan 的内存分配与 Buffer 对象分离，要查询 memoryTypeBits 并绑定兼容内存。

## 修改练习

把矩形改成两个不相连三角形；故意改错 stride 后通过抓帧检查输入。

修改 shader 后重新构建，再运行。Vulkan GLSL 会被编译并复制到 exe 旁的 shaders；DX12 的 HLSL 副本也随构建更新，运行时再编译。应修改源目录而不是运行副本。

## 自测

HOST_COHERENT 是否意味着不用等待 GPU 就可以任意改写缓冲？

参考：不是。它简化 flush/invalidate，不消除 CPU/GPU 同时访问的同步要求。

跨 API 的问题要结合本课 API 的实际对象名称回答；上面题目围绕通用概念，具体 Vulkan 名称可对照 DX12 的 CBV/SRV/UAV、root signature 和 resource barriers。

## 验证和调试

运行 `vk04_vertex_buffer.exe --smoke` 执行四帧（或设备/计算检查）并退出；颜色课会从 GPU 读回 PPM，计算课验证 256 个数。参见仓库 [调试指南](../../../../docs/DEBUGGING.md)。这些基础例子按帧串行等待，重点是可追踪的生命周期；多帧重叠不能仅删除 wait，需要同时改成帧资源环。


## 实测画面

固定时间的 GPU 读回图像（nvpro 为 headless Viewport 输出）。

![vk04_vertex_buffer](../../../../docs/previews/vk04_vertex_buffer.png)
