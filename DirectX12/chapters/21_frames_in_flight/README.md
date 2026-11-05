# 第 21 课 · 两帧资源环与在途使用期 · DirectX 12

[54 课目录](../../COURSE.md) · [对照精讲](../../../docs/compare/09_commands_frames.md) · [上一课](../20_present_sync/README.md) · [下一课](../22_swapchain_recreation/README.md)

## 本课目标与先修

学习目标：解释“两帧资源环与在途使用期”解决的问题，找到实际对象和参数，运行对应实验并用数据/画面证明。先完成前一课，并能说明它交付的对象与完成条件；本课在该依赖上增加新契约。

## 为什么需要这一层

本实验隔离帧槽概念：两组独立缓冲、命令存储与完成标记交替处理八次工作，不在提交第一个槽后立即等待它才提交第二槽。每次重用槽前先等待并核对上一代数据，从结果证明没有覆盖在途资源。

这是实际的双槽 GPU 计算提交，不是已经完成多帧窗口渲染引擎。窗口 renderer 仍保留串行教学路径。将两槽设计推广到窗口时，还要处理交换链图像、depth/target、descriptor、upload 回收和 resize。

## DirectX 12 的对象、参数与执行推导

每槽保存 allocator、command list、资源和完成值。queue Signal 记录本槽提交，下一次重用前 waitValue；一个 ID3D12Fence 通过不同值表达多个完成点。

## 对照 Vulkan 原生：相同问题的另一种分工

每槽有独立 ComputeKernel 输出、command buffer 与 fence，slot=frameNumber%2。先等该槽再 reset/更新，GPU 仍可执行另一个槽的工作；最终检查每槽最后一代结果。

## 关键参数与可检查不变量

| 检查项 | Vulkan 表达 | DX12 表达 | 不变量 / 证据 |
| --- | --- | --- | --- |
| 槽数量 | 2 份 kernel/buffer/cmd/fence | 2 份 resource/allocator/list/value | slot=frameNumber%2 |
| 提交数量 | 8 次计算提交 | 8 次计算提交 | 第二槽提交不先全局等第一槽 |
| 窗口边界 | 窗口仍串行 | 窗口仍串行 | 双槽计算不是完整多帧呈现引擎 |

先为每项指出源码中的创建/绑定位置，以及证明它成立的实际日志、字节或图像。数值中的设备限制以查询为准；固定实验尺寸不当作所有设备的上限。

## nvpro_core2 框架对照

框架有自己的帧循环，但本课程 SceneVk 仍共用一份 UBO 并等待队列。不能因为 Application 支持调度就认为自定义资源已自动变成多帧安全。

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
| DirectX 12 | [dx24_frame_ring](../../lessons/dx24_frame_ring/README.md) |
| Vulkan 原生 | [vk24_frame_ring](../../../Vulkan/native/lessons/vk24_frame_ring/README.md) |

从合集根目录执行；普通图形程序 Esc 退出，设备/计算/诊断程序输出结果后结束。从“开始学习.cmd”进入本页时，表格可直接点击运行。

```powershell
cmake --preset dx12
cmake --build --preset dx12-debug --target dx21_ch_frames_in_flight --parallel 4
.\DirectX12\build\bin\Debug\dx21_ch_frames_in_flight.exe
ctest --preset dx12-debug -R "^dx21_ch_frames_in_flight$" --output-on-failure
```

检查三个层次：创建/绑定/提交的接口约束；数据读回或目标画面；资源使用结束后的回收。诊断实验用合法信息或查询证明渠道可用，不故意保留 API 错误。图形 smoke 的像素检查与数值实验的逐项检查不同，具体实现见源码。

## 关键实现：从当前源码定位

来自 [DirectX12/chapters/21_frames_in_flight/main.cpp](main.cpp#L190)，第 190—206 行。下列是当前仓库的局部切片，可能在函数中间结束；完整上下文请打开文件，切片不作为单独可编译程序。

```cpp
int runChapter(int argc, char** argv) {
    auto options = labOptions(argc, argv, 24);
    DxDevice c(options.warp);
    {
        frameRing(c, options);
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

把槽数改为三，列出所有必须一起改变的数组和完成条件。

先记录预测，再只改一个因素；保留基线与修改后的输出。不要同时改变 shader、资源范围和同步，这样即使结果变好也无法归因。

### 参考推导与通过标准

命令存储、可变缓冲、每槽记录与最终等待均需对应三份。交换链图像数量不必因此变三，imageIndex 仍来自呈现系统。

## 常见失败与排查顺序

只删除 wait 而保持单份 UBO/allocator，可能偶发闪烁或验证错误；延迟出现不代表问题只在最后一帧。

先看第一条错误和当前执行路径，再看实际数据/绑定，最后看 shader 与画面。需要 GPU 证据时抓取正常交互帧；仅四帧的 smoke 适合自动检查，不适合手动抓帧。

## 自测与本课交付

能否不看源码回答：把槽数改为三，列出所有必须一起改变的数组和完成条件。

提交一份简短记录：输入与假设、唯一修改、预期、实际证据、原因、是否恢复基线。如果只能复述函数名而说不清何时允许访问/释放，先回到本课的数据流。

## 官方阅读位置与当前工程差异

[指定 Vulkan 教程对应位置](https://docs.vulkan.net.cn/tutorial/latest/03_Drawing_a_triangle/03_Drawing/03_Frames_in_flight.html)。DX12 路线按同一概念顺序建立映射，其对象与约束以 Microsoft 文档和本后端代码为准。49—54 课包含在基础/compute 之后加入的工程扩展。

[版本与资料说明](../../../docs/REFERENCES.md) · [调试](../../../docs/DEBUGGING.md) · [功能完成度](../../../docs/FEATURE_MATRIX.md)
