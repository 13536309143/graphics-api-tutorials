# nv01_context · Context 与显式资源所有权

[路线首页](../../README.md) · [原生 Vulkan 对照](../../../native/README.md)

## 构建与运行

```powershell
cmake --preset vulkan-nvpro
cmake --build --preset vulkan-nvpro-debug --target nv01_context --parallel 4
.\Vulkan\nvpro\build\bin\Debug\nv01_context.exe
```

以上命令从仓库根目录执行，也可使用本页“运行本课”。预期：nv01_context：只输出设备信息。

## 代码阅读

先读 [main.cpp](main.cpp)，再读路线根目录 common/nvapp_course.hpp 的 NvLesson/nvproMain。框架实现位于 external/nvpro_core2；其固定 commit 和许可证见路线 README。共享底层实现位于 Vulkan/common/scene_vk.hpp，公共数学/几何位于 shared/scene.hpp。

nvvk::Context 根据 ContextInitInfo 创建实例、设备与队列，简化枚举和特性链。本项目固定 Vulkan 1.4，显式启用 validation 与 debug utils。框架对象采用 init/deinit 约定，NvContextOwner 在析构时等待设备并按顺序销毁 messenger/context，避免异常路径忘记清理。框架不会把不支持 1.4 的驱动变成可用驱动。

## 动手练习

在 ContextInitInfo 中申请一条 compute 队列，比较队列族；打印支持和启用的扩展。

自测：列出本课中由 nvpro_core2 管理的对象，以及仍由 SceneVk/应用管理的对象。参考：Context/Application/RenderTarget/Allocator 负责各自对象，shader、pipeline layout、descriptor 与绘制逻辑仍有显式 Vulkan 状态；框架不会自动推断任意自定义 pass 的全部依赖。

## 检查

`nv01_context.exe --smoke` 使用 GPU 无窗口运行或控制台检查。渲染课读回 Viewport 图像为 PPM；计算课逐项验证。验证层错误会使测试失败。常规窗口的 resize/minimize、UI 输入和长时间运行需要另外检查，不能以 headless 通过代替全部交互验证。
