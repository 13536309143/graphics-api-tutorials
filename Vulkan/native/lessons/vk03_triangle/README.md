# vk03_triangle · 动态渲染与第一个三角形

[路线首页](../../README.md) · [上一课](../vk02_swapchain/README.md) · [下一课](../vk04_vertex_buffer/README.md)

## 构建和运行

从仓库根目录执行，完成配置后后续只需 build：

```powershell
cmake --preset vulkan-native
cmake --build --preset vulkan-native-debug --target vk03_triangle --parallel 4
.\Vulkan\native\build\bin\Debug\vk03_triangle.exe
```

也可以通过根目录“开始学习.cmd”打开页面后点击“运行本课”。01/08 是控制台计算/设备检查示例，点击后可用“查看运行输出”阅读结果。其余示例 Esc 退出，拖动窗口可测试尺寸重建。

预期结果：红绿蓝渐变三角形。

## 先读哪些代码

scene_vk.hpp 的 shaderModule/SceneVk pipeline → triangle.vert → nativeMain rendering attachments。

本目录 [main.cpp](main.cpp) 选择本课的执行路径。完整 API 实现在 `common/native_app.hpp 与 ../common/scene_vk.hpp`（相对路线根目录），数学和网格在仓库 shared/scene.hpp。每课都是独立程序；共享代码避免把相同资源生命周期错误复制十遍，也会为后续课预先初始化部分公共状态。首次阅读只沿本课列出的调用路径展开。

## 原理和实现

Vulkan 不会根据当前着色器自动推断完整绘制状态。图形 pipeline 组合 shader、图元拓扑、光栅化、深度与混合状态；pipeline layout 描述 shader 可访问的资源接口。动态渲染通过 VkRenderingInfo 绑定颜色/深度附件，不再预创建传统 VkRenderPass，但附件格式仍须与 pipeline 声明一致。本课三角形位置由 gl_VertexIndex 生成，使用单独的无顶点输入 VS，避免声明了属性却未提供绑定。

## 修改练习

修改 triangle.vert 的颜色和坐标，重建着色器后运行；临时改变拓扑并解释结果。

修改 shader 后重新构建，再运行。Vulkan GLSL 会被编译并复制到 exe 旁的 shaders；DX12 的 HLSL 副本也随构建更新，运行时再编译。应修改源目录而不是运行副本。

## 自测

动态渲染是否可以省略图像布局转换？

参考：不可以，它简化 RenderPass/Framebuffer 对象，不替应用管理资源状态或内存依赖。

跨 API 的问题要结合本课 API 的实际对象名称回答；上面题目围绕通用概念，具体 Vulkan 名称可对照 DX12 的 CBV/SRV/UAV、root signature 和 resource barriers。

## 验证和调试

运行 `vk03_triangle.exe --smoke` 执行四帧（或设备/计算检查）并退出；颜色课会从 GPU 读回 PPM，计算课验证 256 个数。参见仓库 [调试指南](../../../../docs/DEBUGGING.md)。这些基础例子按帧串行等待，重点是可追踪的生命周期；多帧重叠不能仅删除 wait，需要同时改成帧资源环。


## 实测画面

固定时间的 GPU 读回图像（nvpro 为 headless Viewport 输出）。

![vk03_triangle](../../../../docs/previews/vk03_triangle.png)
