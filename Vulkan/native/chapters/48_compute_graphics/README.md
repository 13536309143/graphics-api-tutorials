# 第 48 课 · 计算生成顶点并直接绘制 · Vulkan 原生

[54 课目录](../../COURSE.md) · [对照精讲](../../../../docs/compare/10_compute_readback.md) · [上一课](../47_storage_tail/README.md) · [下一课](../49_group_shared/README.md)

## 本课目标与先修

学习目标：解释“计算生成顶点并直接绘制”解决的问题，找到实际对象和参数，运行对应实验并用数据/画面证明。先完成前一课，并能说明它交付的对象与完成条件；本课在该依赖上增加新契约。

## 为什么需要这一层

96 个粒子各生成三个顶点，GPU 计算写入后直接作为 vertex input 读取。位置不经 CPU 读回再上传，这是真正的 compute→graphics 数据链路；CPU 只提供时间和数量。

## Vulkan 原生 的对象、参数与执行推导

同一 buffer 具有 STORAGE_BUFFER 和 VERTEX_BUFFER usage。计算写入通过 compute-storage-write→vertex-attribute-read 依赖连接绘制。scalar 形式的 storage Vertex 与输入 32 字节 stride 一致，避免 std430 vec3 对齐误配。 直接裁剪坐标在绘制的 Frame 里显式翻转 Y，使数学粒子位置与 DX12 视口映射一致；GPU 生成的数据仍保持同一布局。

## 对照 DirectX 12：相同问题的另一种分工

同一 DEFAULT resource 允许 UAV，计算后转 VERTEX_AND_CONSTANT_BUFFER，VBV 直接引用其地址。计算提交在窗口 renderer 重置 allocator 前完成，当前路径显式等待，未宣称已做异步 compute 重叠。

## 关键参数与可检查不变量

| 检查项 | Vulkan 表达 | DX12 表达 | 不变量 / 证据 |
| --- | --- | --- | --- |
| GPU 输出布局 | 8 scalar float = 32 bytes | Structured Vertex = 32 bytes | storage 与 vertex stride 一致 |
| 粒子规模 | 96×3=288 顶点 / 5 组 | 相同数量 | GPU 生成，CPU 不重传位置 |
| 消费依赖 | compute write→vertex read | UAV→vertex resource state | 当前等待路径不冒充 async compute |

先为每项指出源码中的创建/绑定位置，以及证明它成立的实际日志、字节或图像。数值中的设备限制以查询为准；固定实验尺寸不当作所有设备的上限。

## nvpro_core2 框架对照

框架可提供分配和调度，但必须与自定义 vertex storage 的使用期协调。不能只把 compute 插入 onRender 而不核对命令与资源复用。

这部分是原生 Vulkan 各课的框架对照，框架可运行入口仍为原有六课。如果本节需要新接入工作，会按正文说明；不把原生实验当作已完成的框架实现。

## 本章独立工程与关键注释

[main.cpp](main.cpp) · [本地辅助实现](support/) · [本章着色器](shaders/) · [CMakeLists.txt](CMakeLists.txt)

完整源码就在本目录，关键步骤含中文注释。只复用 GLFW/GLM/Assimp 等第三方库，不包含或链接其他章节的课程实现。单独复制本章文件夹也可配置构建；初始化或纯计算章节不需要图形阶段。

```powershell
cmake -S . -B build -A x64
cmake --build build --config Debug --parallel 4
ctest --test-dir build -C Debug --output-on-failure
```

## 运行与实际检查

| 路线 | 实际实验 |
| --- | --- |
| Vulkan 原生 | [vk29_gpu_particles](../../lessons/vk29_gpu_particles/README.md) |
| DirectX 12 | [dx29_gpu_particles](../../../../DirectX12/lessons/dx29_gpu_particles/README.md) |

从合集根目录执行；普通图形程序 Esc 退出，设备/计算/诊断程序输出结果后结束。从“开始学习.cmd”进入本页时，表格可直接点击运行。

```powershell
cmake --preset vulkan-native
cmake --build --preset vulkan-native-debug --target vk48_ch_compute_graphics --parallel 4
.\Vulkan\native\build\bin\Debug\vk48_ch_compute_graphics.exe
ctest --preset vulkan-native-debug -R "^vk48_ch_compute_graphics$" --output-on-failure
```

检查三个层次：创建/绑定/提交的接口约束；数据读回或目标画面；资源使用结束后的回收。诊断实验用合法信息或查询证明渠道可用，不故意保留 API 错误。图形 smoke 的像素检查与数值实验的逐项检查不同，具体实现见源码。

## 关键实现：从当前源码定位

来自 [Vulkan/native/chapters/48_compute_graphics/main.cpp](main.cpp#L166)，第 166—180 行。下列是当前仓库的局部切片，可能在函数中间结束；完整上下文请打开文件，切片不作为单独可编译程序。

```cpp
int runChapter(int argc, char** argv) {
    auto options = labOptions(argc, argv, 29);
    nativeShaders = options.directory / "shaders";
    {
        return graphicsLab(argc, argv, options);
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

阅读时按“前置对象 → 参数/布局 → 录制/调用 → 完成/可见性 → 释放”五步标注。辅助实现与着色器是本章自己的文件；修改不会影响其他章。

## 受控修改实验

把粒子数改为 97，需要检查哪些容量与 dispatch/guard？

先记录预测，再只改一个因素；保留基线与修改后的输出。不要同时改变 shader、资源范围和同步，这样即使结果变好也无法归因。

### 参考推导与通过标准

顶点数变为 291，GPU buffer 与 CPU 侧 draw count 元数据都要扩大；组数向上取整，shader 以 count*3 为上界。位置仍由 GPU 生成。

## 常见失败与排查顺序

buffer 转为可绘制用途并不意味着内存布局自动相同；vec3 填充与结构化 stride 错误会使粒子变成刺状几何。

先看第一条错误和当前执行路径，再看实际数据/绑定，最后看 shader 与画面。需要 GPU 证据时抓取正常交互帧；仅四帧的 smoke 适合自动检查，不适合手动抓帧。

## 自测与本课交付

能否不看源码回答：把粒子数改为 97，需要检查哪些容量与 dispatch/guard？

提交一份简短记录：输入与假设、唯一修改、预期、实际证据、原因、是否恢复基线。如果只能复述函数名而说不清何时允许访问/释放，先回到本课的数据流。

## 官方阅读位置与当前工程差异

[指定 Vulkan 教程对应位置](https://docs.vulkan.net.cn/tutorial/latest/11_Compute_Shader.html)。DX12 路线按同一概念顺序建立映射，其对象与约束以 Microsoft 文档和本后端代码为准。49—54 课包含在基础/compute 之后加入的工程扩展。

[版本与资料说明](../../../../docs/REFERENCES.md) · [调试](../../../../docs/DEBUGGING.md) · [功能完成度](../../../../docs/FEATURE_MATRIX.md)

## 固定输入的实际 GPU 读回

![vk48_ch_compute_graphics](../../../../docs/previews/vk29_gpu_particles.png)
