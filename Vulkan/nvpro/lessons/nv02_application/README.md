# nv02_application · nvapp Application 生命周期

[路线首页](../../README.md) · [原生 Vulkan 对照](../../../native/README.md)

## 构建与运行

```powershell
cmake --preset vulkan-nvpro
cmake --build --preset vulkan-nvpro-debug --target nv02_application --parallel 4
.\Vulkan\nvpro\build\bin\Debug\nv02_application.exe
```

以上命令从仓库根目录执行，也可使用本页“运行本课”。预期：深蓝色 Viewport，可调窗口布局。

## 代码阅读

先读 [main.cpp](main.cpp)，再读路线根目录 common/nvapp_course.hpp 的 NvLesson/nvproMain。框架实现位于 external/nvpro_core2；其固定 commit 和许可证见路线 README。共享底层实现位于 Vulkan/common/scene_vk.hpp，公共数学/几何位于 shared/scene.hpp。

nvapp::Application 管理窗口、交换链、UI 和帧提交，IAppElement 提供 onAttach/onResize/onUIRender/onRender/onDetach。渲染不直接写交换链，而是写 nvvk::RenderTarget，再交给 ImGui 显示。onResize 中更新附件与显示描述符；资源必须先于 allocator/device 销毁。--smoke 使用官方 headless 路径，仍进行真实 GPU 渲染。

## 动手练习

修改清屏色；在每个回调中打印一次调用顺序；缩放 Viewport 观察 update。

自测：列出本课中由 nvpro_core2 管理的对象，以及仍由 SceneVk/应用管理的对象。参考：Context/Application/RenderTarget/Allocator 负责各自对象，shader、pipeline layout、descriptor 与绘制逻辑仍有显式 Vulkan 状态；框架不会自动推断任意自定义 pass 的全部依赖。

## 检查

`nv02_application.exe --smoke` 使用 GPU 无窗口运行或控制台检查。渲染课读回 Viewport 图像为 PPM；计算课逐项验证。验证层错误会使测试失败。常规窗口的 resize/minimize、UI 输入和长时间运行需要另外检查，不能以 headless 通过代替全部交互验证。


## 实测画面

固定时间的 GPU 读回图像（nvpro 为 headless Viewport 输出）。

![nv02_application](../../../../docs/previews/nv02_application.png)
