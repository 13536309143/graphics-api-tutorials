# vk10_offscreen · 离屏目标与图像传输

[路线首页](../../README.md) · [上一课](../vk09_pbr/README.md) · [下一课](../../README.md)

## 构建和运行

从仓库根目录执行，完成配置后后续只需 build：

```powershell
cmake --preset vulkan-native
cmake --build --preset vulkan-native-debug --target vk10_offscreen --parallel 4
.\Vulkan\native\build\bin\Debug\vk10_offscreen.exe
```

也可以通过根目录“开始学习.cmd”打开页面后点击“运行本课”。01/08 是控制台计算/设备检查示例，点击后可用“查看运行输出”阅读结果。其余示例 Esc 退出，拖动窗口可测试尺寸重建。

预期结果：PBR 先画进独立 RGBA8 目标，再 blit 到交换链显示。

## 先读哪些代码

Swapchain offscreen → SceneVk(format=RGBA8) → COLOR_ATTACHMENT→TRANSFER_SRC → vkCmdBlitImage。

本目录 [main.cpp](main.cpp) 选择本课的执行路径。完整 API 实现在 `common/native_app.hpp 与 ../common/scene_vk.hpp`（相对路线根目录），数学和网格在仓库 shared/scene.hpp。每课都是独立程序；共享代码避免把相同资源生命周期错误复制十遍，也会为后续课预先初始化部分公共状态。首次阅读只沿本课列出的调用路径展开。

## 原理和实现

本课把可见性与中间渲染结果拆开：pipeline 输出格式与离屏图一致，再按交换链格式转换/复制。图像 blit 与同格式 copy 不同，需要检查传输 usage 和设备格式支持。当前使用单采样同尺寸颜色，便于核对状态；没有额外色调后处理 pass。CPU 测试再把最终交换链图像复制到 readback buffer，等待完成后写 PPM。

## 修改练习

把离屏目标缩为一半尺寸，再用线性 blit 放大，并检查 format features；扩展单独的全屏后处理 pipeline。

修改 shader 后重新构建，再运行。Vulkan GLSL 会被编译并复制到 exe 旁的 shaders；DX12 的 HLSL 副本也随构建更新，运行时再编译。应修改源目录而不是运行副本。

## 自测

为什么不能给一个缺少 TRANSFER_SRC usage 的图像随意执行 blit？

参考：usage 是创建资源时对合法用途的声明，后续命令必须符合声明和设备格式能力。

跨 API 的问题要结合本课 API 的实际对象名称回答；上面题目围绕通用概念，具体 Vulkan 名称可对照 DX12 的 CBV/SRV/UAV、root signature 和 resource barriers。

## 验证和调试

运行 `vk10_offscreen.exe --smoke` 执行四帧（或设备/计算检查）并退出；颜色课会从 GPU 读回 PPM，计算课验证 256 个数。参见仓库 [调试指南](../../../../docs/DEBUGGING.md)。这些基础例子按帧串行等待，重点是可追踪的生命周期；多帧重叠不能仅删除 wait，需要同时改成帧资源环。


## 实测画面

固定时间的 GPU 读回图像（nvpro 为 headless Viewport 输出）。

![vk10_offscreen](../../../../docs/previews/vk10_offscreen.png)
