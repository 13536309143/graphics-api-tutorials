# nv03_triangle · 动态渲染与共享场景

[路线首页](../../README.md) · [原生 Vulkan 对照](../../../native/README.md)

## 构建与运行

```powershell
cmake --preset vulkan-nvpro
cmake --build --preset vulkan-nvpro-debug --target nv03_triangle --parallel 4
.\Vulkan\nvpro\build\bin\Debug\nv03_triangle.exe
```

以上命令从仓库根目录执行，也可使用本页“运行本课”。预期：Viewport 显示彩色三角形。

## 代码阅读

先读 [main.cpp](main.cpp)，再读路线根目录 common/nvapp_course.hpp 的 NvLesson/nvproMain。框架实现位于 external/nvpro_core2；其固定 commit 和许可证见路线 README。共享底层实现位于 Vulkan/common/scene_vk.hpp，公共数学/几何位于 shared/scene.hpp。

nvvk::RenderTargetState 把附件状态整理为动态渲染信息，cmdBeginRendering 开始 pass；底层图形 pipeline 与 shader 仍通过 Vulkan 代码构建。此处刻意与 native 共用 SceneVk，方便把“框架负责窗口/目标”和“应用仍负责 shader/绘制状态”区分开。GENERAL 布局不代表可以省略图像写后读屏障。

## 动手练习

读 RenderTargetState::cmdBeginRendering，对照原生 VkRenderingInfo；把 Viewport 内容保存并核对读回。

自测：列出本课中由 nvpro_core2 管理的对象，以及仍由 SceneVk/应用管理的对象。参考：Context/Application/RenderTarget/Allocator 负责各自对象，shader、pipeline layout、descriptor 与绘制逻辑仍有显式 Vulkan 状态；框架不会自动推断任意自定义 pass 的全部依赖。

## 检查

`nv03_triangle.exe --smoke` 使用 GPU 无窗口运行或控制台检查。渲染课读回 Viewport 图像为 PPM；计算课逐项验证。验证层错误会使测试失败。常规窗口的 resize/minimize、UI 输入和长时间运行需要另外检查，不能以 headless 通过代替全部交互验证。


## 实测画面

固定时间的 GPU 读回图像（nvpro 为 headless Viewport 输出）。

![nv03_triangle](../../../../docs/previews/nv03_triangle.png)
