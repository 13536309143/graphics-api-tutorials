# vk08_compute_readback · Compute、屏障与 GPU 读回

[路线首页](../../README.md) · [上一课](../vk07_depth_instancing/README.md) · [下一课](../vk09_pbr/README.md)

## 构建和运行

从仓库根目录执行，完成配置后后续只需 build：

```powershell
cmake --preset vulkan-native
cmake --build --preset vulkan-native-debug --target vk08_compute_readback --parallel 4
.\Vulkan\native\build\bin\Debug\vk08_compute_readback.exe
```

也可以通过根目录“开始学习.cmd”打开页面后点击“运行本课”。01/08 是控制台计算/设备检查示例，点击后可用“查看运行输出”阅读结果。其余示例 Esc 退出，拖动窗口可测试尺寸重建。

预期结果：无窗口，在 GPU 计算 256 个 i*i+1 并逐项验证。

## 先读哪些代码

computeSquares → storage descriptor → square.comp → dispatch → compute-to-host barrier → fence → map。

本目录 [main.cpp](main.cpp) 选择本课的执行路径。完整 API 实现在 `common/native_app.hpp 与 ../common/scene_vk.hpp`（相对路线根目录），数学和网格在仓库 shared/scene.hpp。每课都是独立程序；共享代码避免把相同资源生命周期错误复制十遍，也会为后续课预先初始化部分公共状态。首次阅读只沿本课列出的调用路径展开。

## 原理和实现

本课不靠画面确认 compute，直接验证 256 个结果。local_size_x=64，dispatch(4,1,1) 覆盖 256 次调用。SSBO 的 shader 写入之后，通过 synchronization2 声明 COMPUTE_SHADER/SHADER_STORAGE_WRITE 到 HOST/HOST_READ，再等待 fence 后读取映射的 coherent 内存。执行完成与内存可见性是两个需要分别说明的概念；如果使用非 coherent 内存，还应 invalidate。

## 修改练习

改成向量加法并逐项验证；测试数量不是 64 倍数时的边界条件。

修改 shader 后重新构建，再运行。Vulkan GLSL 会被编译并复制到 exe 旁的 shaders；DX12 的 HLSL 副本也随构建更新，运行时再编译。应修改源目录而不是运行副本。

## 自测

只 map 缓冲而不等待 fence，读到的是什么？

参考：可能是旧值、未完成结果或与 GPU 发生数据竞争，不能视为合法结果。

跨 API 的问题要结合本课 API 的实际对象名称回答；上面题目围绕通用概念，具体 Vulkan 名称可对照 DX12 的 CBV/SRV/UAV、root signature 和 resource barriers。

## 验证和调试

运行 `vk08_compute_readback.exe --smoke` 执行四帧（或设备/计算检查）并退出；颜色课会从 GPU 读回 PPM，计算课验证 256 个数。参见仓库 [调试指南](../../../../docs/DEBUGGING.md)。这些基础例子按帧串行等待，重点是可追踪的生命周期；多帧重叠不能仅删除 wait，需要同时改成帧资源环。
