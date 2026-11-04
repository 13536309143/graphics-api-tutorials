# nv05_compute_readback · Compute 与读回验证

[路线首页](../../README.md) · [原生 Vulkan 对照](../../../native/README.md)

## 构建与运行

```powershell
cmake --preset vulkan-nvpro
cmake --build --preset vulkan-nvpro-debug --target nv05_compute_readback --parallel 4
.\Vulkan\nvpro\build\bin\Debug\nv05_compute_readback.exe
```

以上命令从仓库根目录执行，也可使用本页“运行本课”。预期：无窗口，在 context 队列完成 256 项计算并验证。

## 代码阅读

先读 [main.cpp](main.cpp)，再读路线根目录 common/nvapp_course.hpp 的 NvLesson/nvproMain。框架实现位于 external/nvpro_core2；其固定 commit 和许可证见路线 README。共享底层实现位于 Vulkan/common/scene_vk.hpp，公共数学/几何位于 shared/scene.hpp。

在 nvpro 管理的 device/queue 上调用与原生课程相同的 computeSquares。这样可逐项验证 framework context 没有改变 shader binding、dispatch 数量和 host 可见性规则。当前保留原生 SSBO/descriptor/pipeline 代码，以便清楚比较上下文封装前后的边界；不是把一段 CPU 运算包装成框架 compute。

## 动手练习

把存储/读回缓冲改为 VMA mapped buffer，保留 flush/invalidate 和 fence 正确性。

自测：列出本课中由 nvpro_core2 管理的对象，以及仍由 SceneVk/应用管理的对象。参考：Context/Application/RenderTarget/Allocator 负责各自对象，shader、pipeline layout、descriptor 与绘制逻辑仍有显式 Vulkan 状态；框架不会自动推断任意自定义 pass 的全部依赖。

## 检查

`nv05_compute_readback.exe --smoke` 使用 GPU 无窗口运行或控制台检查。渲染课读回 Viewport 图像为 PPM；计算课逐项验证。验证层错误会使测试失败。常规窗口的 resize/minimize、UI 输入和长时间运行需要另外检查，不能以 headless 通过代替全部交互验证。
