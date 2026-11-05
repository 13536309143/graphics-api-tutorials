# dx08_compute_uav · Compute UAV 与 GPU 读回

## 本课独立工程与关键注释

[main.cpp](main.cpp) · [本地辅助实现](support/) · [本课着色器](shaders/) · [CMakeLists.txt](CMakeLists.txt)

本目录拥有完整课程源码、资源与依赖配方。关键步骤含中文注释，不链接其他课的课程实现。可将整个目录复制到其他位置，用以下命令单独构建和检查；第三方依赖首次配置需要联网或提供缓存。

```powershell
cmake -S . -B build -A x64
cmake --build build --config Debug --parallel 4
ctest --test-dir build -C Debug --output-on-failure
```

[路线首页](../../README.md) · [上一课](../dx07_depth_instancing/README.md) · [下一课](../dx09_pbr/README.md)

## 构建和运行

从仓库根目录执行，完成配置后后续只需 build：

```powershell
cmake --preset dx12
cmake --build --preset dx12-debug --target dx08_compute_uav --parallel 4
.\DirectX12\build\bin\Debug\dx08_compute_uav.exe
```

也可以通过根目录“开始学习.cmd”打开页面后点击“运行本课”。01/08 是控制台计算/设备检查示例，点击后可用“查看运行输出”阅读结果。其余示例 Esc 退出，拖动窗口可测试尺寸重建。

预期结果：无窗口，在 GPU 计算 256 个 i*i+1 并逐项验证。

## 先读哪些代码

Compute Root UAV / Dispatch / Readback

本目录 [main.cpp](main.cpp) 选择本课的执行路径。完整 API 实现在 `common/dx12_course.hpp`（相对路线根目录），数学和网格在仓库 shared/scene.hpp。每课都是独立程序；共享代码避免把相同资源生命周期错误复制十遍，也会为后续课预先初始化部分公共状态。首次阅读只沿本课列出的调用路径展开。

## 原理和实现

计算缓冲放在 default heap 并允许 UAV，root UAV 传 GPU 地址。Dispatch 后转换 UAV→COPY_SOURCE 的屏障建立依赖，再复制到 READBACK heap，等待 fence 后 map。256 个值全部逐项验证，不只看第一个数。

本课使用与 Vulkan 相同的几何和验证目标，比较资源绑定与同步的不同表达。CPU 创建的数据、GPU 资源和用于访问它的 view/descriptor 是不同层次；用调试层与抓帧工具分别核对接口约定和实际内容。命令异步执行，示例显式等待 fence 后才重用单份上传资源与 command allocator。

HLSL 使用 column_major 与 mul(M,v)，保持与共享 GLM 数据相同的内存和乘法约定。调整窗口大小时必须更新尺寸相关的目标与投影；map、创建视图或改变 CPU 变量都不会替代已经提交给 GPU 的工作。

## 修改练习

把计算改成两个缓冲相加并逐项比较 CPU 参考；尝试非 64 倍数的调用数量。

修改 shader 后重新构建，再运行。Vulkan GLSL 会被编译并复制到 exe 旁的 shaders；DX12 的 HLSL 副本也随构建更新，运行时再编译。应修改源目录而不是运行副本。

## 自测

为什么 readback 前要等 fence？

参考：复制和计算都异步执行，map 不代表 GPU 已写完。


## 验证和调试

运行 `dx08_compute_uav.exe --smoke` 执行四帧（或设备/计算检查）并退出；颜色课会从 GPU 读回 PPM，计算课验证 256 个数。参见仓库 [调试指南](../../../docs/DEBUGGING.md)。这些基础例子按帧串行等待，重点是可追踪的生命周期；多帧重叠不能仅删除 wait，需要同时改成帧资源环。


## 深入学习：数据、接口与执行

计算写 DEFAULT UAV 后转 COPY_SOURCE，再拷 READBACK 并等待 fence。Map 的 read range 表示 CPU 将读哪些字节，不会自己触发 GPU 等待。

配套精讲：[三 API 对照 · 计算与读回](../../../docs/compare/10_compute_readback.md)。先读概念与参数选择，再返回本课实现核对，不必一次展开所有公共类。

### 本课的真实源码切片

来自 [DirectX12/lessons/dx08_compute_uav/main.cpp](main.cpp#L9)，第 9—35 行；这是现有实现的局部摘录，末尾可能处于函数中间，不是可单独编译的完整程序。

```cpp
int runChapter(int argc, char** argv) {
    DxRenderOptions options;

    dxShaderFile = std::filesystem::absolute(argv[0]).parent_path() / COURSE_RUNTIME_NAME /
                   "shaders" / "scene.hlsl";
    bool smoke = false, warp = false;
    for (int i = 1; i < argc; ++i) {
        smoke |= std::strcmp(argv[i], "--smoke") == 0;
        warp |= std::strcmp(argv[i], "--warp") == 0;
    }
    {
        DxDevice c(warp);
        {
            dxCompute(c);
        }
        c.checkMessages();
        return 0;
    }
}
} // namespace course

int main(int argc, char** argv) try {
    return course::runChapter(argc, argv);
} catch (const std::exception& error) {
    std::cerr << "ERROR: " << error.what() << '\n';
    return 1;
}
```

### 同主题跨 API 导航

以下是同主题参考；场景、功能完成度和版本基线可能不同，具体边界见对照章节。

| 路线 | 可运行参照 / 当前范围 |
| --- | --- |
| OpenGL | 基线外：当前 OpenGL 3.3 路线没有 compute 实现；粒子实例化不能作为 compute 对照。 |
| Vulkan 原生 | [vk08_compute_readback](../../../Vulkan/native/lessons/vk08_compute_readback/README.md) |
| Vulkan / nvpro_core2 | [nv05_compute_readback](../../../Vulkan/nvpro/lessons/nv05_compute_readback/README.md) |
| DirectX 12 | [dx08_compute_uav](README.md) |

### 先预测，再验证

能把 DEFAULT resource 直接当作当前 READBACK 来 map 吗？

参考推导：不能以函数同名推断内存能力。按 heap 类型和资源规则选择路径，再管理 GPU 完成与读取范围。

在一次实验中只改一个因素，记录预测、实际画面/日志和原因。检查已有 smoke 是否仍通过；图形调试器里再核对本次 draw 的实际输入/输出，不能仅凭进程正常退出下结论。
