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


## 验证和调试

运行 `vk08_compute_readback.exe --smoke` 执行四帧（或设备/计算检查）并退出；颜色课会从 GPU 读回 PPM，计算课验证 256 个数。参见仓库 [调试指南](../../../../docs/DEBUGGING.md)。这些基础例子按帧串行等待，重点是可追踪的生命周期；多帧重叠不能仅删除 wait，需要同时改成帧资源环。


## 深入学习：数据、接口与执行

storage buffer 收集 256 项，dispatch 后声明 compute-write 到 host-read，提交等待完成再 map。HOST_COHERENT 只简化缓存管理，不能允许 CPU 抢先读取。

配套精讲：[三 API 对照 · 计算与读回](../../../../docs/compare/10_compute_readback.md)。先读概念与参数选择，再返回本课实现核对，不必一次展开所有公共类。

### 本课的真实源码切片

来自 [Vulkan/common/shaders/square.comp](../../../common/shaders/square.comp#L2)，第 2—12 行；这是现有实现的局部摘录，末尾可能处于函数中间，不是可单独编译的完整程序。

```glsl
layout(local_size_x = 64) in;
layout(binding = 0) buffer Values {
    uint values[];
};

// 每工作组 64 个线程；host dispatch 4 组，结果通过 storage buffer 读回。
void main() {
    uint i = gl_GlobalInvocationID.x;
    if (i < 256)
        values[i] = i * i + 1;
}
```

### 同主题跨 API 导航

以下是同主题参考；场景、功能完成度和版本基线可能不同，具体边界见对照章节。

| 路线 | 可运行参照 / 当前范围 |
| --- | --- |
| OpenGL | 基线外：当前 OpenGL 3.3 路线没有 compute 实现；粒子实例化不能作为 compute 对照。 |
| Vulkan 原生 | [vk08_compute_readback](README.md) |
| Vulkan / nvpro_core2 | [nv05_compute_readback](../../../nvpro/lessons/nv05_compute_readback/README.md) |
| DirectX 12 | [dx08_compute_uav](../../../../DirectX12/lessons/dx08_compute_uav/README.md) |

### 先预测，再验证

只看 result[0] 为什么不足？

参考推导：线程索引或组数错误仍可能让第零项正确。逐项核对才能发现未覆盖尾部、重复写入和范围错误。

在一次实验中只改一个因素，记录预测、实际画面/日志和原因。检查已有 smoke 是否仍通过；图形调试器里再核对本次 draw 的实际输入/输出，不能仅凭进程正常退出下结论。
