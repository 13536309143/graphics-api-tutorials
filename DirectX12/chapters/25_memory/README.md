# 第 25 课 · 内存类型、Heap 与分配要求 · DirectX 12

[54 课目录](../../COURSE.md) · [对照精讲](../../../docs/compare/03_geometry_memory.md) · [上一课](../24_vertex_buffer/README.md) · [下一课](../26_staging/README.md)

## 本课目标与先修

学习目标：解释“内存类型、Heap 与分配要求”解决的问题，找到实际对象和参数，运行对应实验并用数据/画面证明。先完成前一课，并能说明它交付的对象与完成条件；本课在该依赖上增加新契约。

## 为什么需要这一层

资源逻辑大小、分配大小和偏移对齐不是同一个数字。API 可能要求更大或对齐后的存储。可被 CPU 访问也不等于最适合 GPU 高频读取，统一内存设备与独立显卡更不能凭经验写死同一选择。

## DirectX 12 的对象、参数与执行推导

GetResourceAllocationInfo 解释实际分配大小/对齐，UPLOAD/DEFAULT/READBACK 表达常见用途。GetCustomHeapProperties 可以帮助理解架构映射，但不能随意绕过 heap/resource 规则。CBV 的 256 字节地址对齐与普通 buffer 分配对齐也不是同一要求。

## 对照 Vulkan 原生：相同问题的另一种分工

memoryTypeBits 是资源允许的类型集合，propertyFlags 是内存属性。实验为 257 字节缓冲查询实际 size/alignment/type bits，再在允许集合中选择所需属性。non-coherent 的 flush/invalidate 还需符合原子尺寸对齐。

## 关键参数与可检查不变量

| 检查项 | Vulkan 表达 | DX12 表达 | 不变量 / 证据 |
| --- | --- | --- | --- |
| 逻辑与实际大小 | buffer size 与 MemoryRequirements | resource size 与 AllocationInfo | 实际分配可能大于请求 |
| 选择约束 | memoryTypeBits 与属性交集 | heap/resource 兼容规则 | 不能仅按类型编号猜位置 |
| 接口对齐 | 设备 UBO offset alignment | CBV 256 bytes | 不同于一般 allocation alignment |

先为每项指出源码中的创建/绑定位置，以及证明它成立的实际日志、字节或图像。数值中的设备限制以查询为准；固定实验尺寸不当作所有设备的上限。

## nvpro_core2 框架对照

VMA 从重复分配逻辑中抽象出 allocator，但 flags、访问目标与释放时机仍由调用方决定。不要把 allocator 当成无条件允许 Map 的通行证。

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
| DirectX 12 | [dx47_allocation_requirements](../../lessons/dx47_allocation_requirements/README.md) |
| DirectX 12 | [dx14_constant_alignment](../../lessons/dx14_constant_alignment/README.md) |
| Vulkan 原生 | [vk47_allocation_requirements](../../../Vulkan/native/lessons/vk47_allocation_requirements/README.md) |
| Vulkan 原生 | [vk14_constant_alignment](../../../Vulkan/native/lessons/vk14_constant_alignment/README.md) |

从合集根目录执行；普通图形程序 Esc 退出，设备/计算/诊断程序输出结果后结束。从“开始学习.cmd”进入本页时，表格可直接点击运行。

```powershell
cmake --preset dx12
cmake --build --preset dx12-debug --target dx25_ch_memory --parallel 4
.\DirectX12\build\bin\Debug\dx25_ch_memory.exe
ctest --preset dx12-debug -R "^dx25_ch_memory$" --output-on-failure
```

检查三个层次：创建/绑定/提交的接口约束；数据读回或目标画面；资源使用结束后的回收。诊断实验用合法信息或查询证明渠道可用，不故意保留 API 错误。图形 smoke 的像素检查与数值实验的逐项检查不同，具体实现见源码。

## 关键实现：从当前源码定位

来自 [DirectX12/chapters/25_memory/main.cpp](main.cpp#L32)，第 32—48 行。下列是当前仓库的局部切片，可能在函数中间结束；完整上下文请打开文件，切片不作为单独可编译程序。

```cpp
int runChapter(int argc, char** argv) {
    auto options = labOptions(argc, argv, 47);
    DxDevice c(options.warp);
    {
        inspection(c, options);
    }
    c.checkMessages();
    return 0;
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

比较请求 257 字节、返回的 allocation size、常量槽 stride 三者，解释不同来源。

先记录预测，再只改一个因素；保留基线与修改后的输出。不要同时改变 shader、资源范围和同步，这样即使结果变好也无法归因。

### 参考推导与通过标准

257 是应用内容大小，allocation size 来自内存分配约束，常量 stride 来自接口偏移对齐。三者可以不同而同时正确。

## 常见失败与排查顺序

先分配“看起来够大”的字节再忽略 memoryTypeBits，或把 GPU 可写默认纹理直接 Map，都是层次混淆。

先看第一条错误和当前执行路径，再看实际数据/绑定，最后看 shader 与画面。需要 GPU 证据时抓取正常交互帧；仅四帧的 smoke 适合自动检查，不适合手动抓帧。

## 自测与本课交付

能否不看源码回答：比较请求 257 字节、返回的 allocation size、常量槽 stride 三者，解释不同来源。

提交一份简短记录：输入与假设、唯一修改、预期、实际证据、原因、是否恢复基线。如果只能复述函数名而说不清何时允许访问/释放，先回到本课的数据流。

## 官方阅读位置与当前工程差异

[指定 Vulkan 教程对应位置](https://docs.vulkan.net.cn/tutorial/latest/04_Vertex_buffers/01_Vertex_buffer_creation.html)。DX12 路线按同一概念顺序建立映射，其对象与约束以 Microsoft 文档和本后端代码为准。49—54 课包含在基础/compute 之后加入的工程扩展。

[版本与资料说明](../../../docs/REFERENCES.md) · [调试](../../../docs/DEBUGGING.md) · [功能完成度](../../../docs/FEATURE_MATRIX.md)
