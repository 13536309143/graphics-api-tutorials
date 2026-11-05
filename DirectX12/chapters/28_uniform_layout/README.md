# 第 28 课 · 常量结构、矩阵存储与对齐 · DirectX 12

[54 课目录](../../COURSE.md) · [对照精讲](../../../docs/compare/05_binding_constants.md) · [上一课](../27_indices/README.md) · [下一课](../29_descriptor_layout/README.md)

## 本课目标与先修

学习目标：解释“常量结构、矩阵存储与对齐”解决的问题，找到实际对象和参数，运行对应实验并用数据/画面证明。先完成前一课，并能说明它交付的对象与完成条件；本课在该依赖上增加新契约。

## 为什么需要这一层

跨 CPU/shader 的二进制契约应可检查：Frame=64 字节，Draw=80 字节，settings 偏移=64。矩阵存储顺序、成员对齐、接口范围与偏移对齐分别证明，不靠 alignas 一词概括所有规则。

## DirectX 12 的对象、参数与执行推导

HLSL column_major 与 mul(M,v) 配合 GLM。root CBV 的 GPU 地址需要 256 字节对齐，实际帧矩阵仍只有 64 字节；根常量按 DWORD 更新。不要把所有 C++ struct 都假定与 cbuffer 打包自动一致。

## 对照 Vulkan 原生：相同问题的另一种分工

GLSL mat4 与 GLM 列主序配合，UBO 的布局必须与 C++ 实际偏移一致。动态偏移按 minUniformBufferOffsetAlignment 对齐，range 仍应覆盖具体内容且不越界。实验用 uvec4 参数让 GPU 数值验证解码。

## 关键参数与可检查不变量

| 检查项 | Vulkan 表达 | DX12 表达 | 不变量 / 证据 |
| --- | --- | --- | --- |
| 二进制布局 | Frame=64 / Draw=80 | Frame=64 / Draw=80 | settings 偏移=64 |
| 矩阵约定 | GLM 列主序 / mat4 | column_major / mul(M,v) | 存储与乘法记法分别核对 |
| 常量偏移 | 动态 offset 按设备对齐 | root CBV 地址 256 对齐 | alignas(16) 不覆盖全部接口规则 |

先为每项指出源码中的创建/绑定位置，以及证明它成立的实际日志、字节或图像。数值中的设备限制以查询为准；固定实验尺寸不当作所有设备的上限。

## nvpro_core2 框架对照

共享 Frame/Draw 与相同 shader，没有另一种框架专用矩阵规则。增加骨骼或其他常量时仍需新的布局与容量验证。

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
| DirectX 12 | [dx51_uniform_layout](../../lessons/dx51_uniform_layout/README.md) |
| DirectX 12 | [dx14_constant_alignment](../../lessons/dx14_constant_alignment/README.md) |
| Vulkan 原生 | [vk51_uniform_layout](../../../Vulkan/native/lessons/vk51_uniform_layout/README.md) |
| Vulkan 原生 | [vk14_constant_alignment](../../../Vulkan/native/lessons/vk14_constant_alignment/README.md) |

从合集根目录执行；普通图形程序 Esc 退出，设备/计算/诊断程序输出结果后结束。从“开始学习.cmd”进入本页时，表格可直接点击运行。

```powershell
cmake --preset dx12
cmake --build --preset dx12-debug --target dx28_ch_uniform_layout --parallel 4
.\DirectX12\build\bin\Debug\dx28_ch_uniform_layout.exe
ctest --preset dx12-debug -R "^dx28_ch_uniform_layout$" --output-on-failure
```

检查三个层次：创建/绑定/提交的接口约束；数据读回或目标画面；资源使用结束后的回收。诊断实验用合法信息或查询证明渠道可用，不故意保留 API 错误。图形 smoke 的像素检查与数值实验的逐项检查不同，具体实现见源码。

## 关键实现：从当前源码定位

来自 [DirectX12/chapters/28_uniform_layout/main.cpp](main.cpp#L147)，第 147—163 行。下列是当前仓库的局部切片，可能在函数中间结束；完整上下文请打开文件，切片不作为单独可编译程序。

```cpp
int runChapter(int argc, char** argv) {
    auto options = labOptions(argc, argv, 51);
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

在 CPU 打印 sizeof/offsetof，再通过 GPU 计算验证第三个常量槽的 factor/bias。

先记录预测，再只改一个因素；保留基线与修改后的输出。不要同时改变 shader、资源范围和同步，这样即使结果变好也无法归因。

### 参考推导与通过标准

CPU 布局断言检查内存，GPU 结果检查接口选择与解码；两份证据互补。第三槽得到 i*7+13，才说明该范围被正确使用。

## 常见失败与排查顺序

用 vec3/float3 扩展结构却不复查 padding，或认为 transpose 能同时修复存储与数学语义，会造成隐蔽错误。

先看第一条错误和当前执行路径，再看实际数据/绑定，最后看 shader 与画面。需要 GPU 证据时抓取正常交互帧；仅四帧的 smoke 适合自动检查，不适合手动抓帧。

## 自测与本课交付

能否不看源码回答：在 CPU 打印 sizeof/offsetof，再通过 GPU 计算验证第三个常量槽的 factor/bias。

提交一份简短记录：输入与假设、唯一修改、预期、实际证据、原因、是否恢复基线。如果只能复述函数名而说不清何时允许访问/释放，先回到本课的数据流。

## 官方阅读位置与当前工程差异

[指定 Vulkan 教程对应位置](https://docs.vulkan.net.cn/tutorial/latest/05_Uniform_buffers/00_Descriptor_set_layout_and_buffer.html)。DX12 路线按同一概念顺序建立映射，其对象与约束以 Microsoft 文档和本后端代码为准。49—54 课包含在基础/compute 之后加入的工程扩展。

[版本与资料说明](../../../docs/REFERENCES.md) · [调试](../../../docs/DEBUGGING.md) · [功能完成度](../../../docs/FEATURE_MATRIX.md)
