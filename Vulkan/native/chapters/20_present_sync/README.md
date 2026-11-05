# 第 20 课 · 获取、提交、呈现与完成标记 · Vulkan 原生

[54 课目录](../../COURSE.md) · [对照精讲](../../../../docs/compare/08_synchronization.md) · [上一课](../19_record_submit/README.md) · [下一课](../21_frames_in_flight/README.md)

## 本课目标与先修

学习目标：解释“获取、提交、呈现与完成标记”解决的问题，找到实际对象和参数，运行对应实验并用数据/画面证明。先完成前一课，并能说明它交付的对象与完成条件；本课在该依赖上增加新契约。

## 为什么需要这一层

显示一帧涉及“图像可写”“渲染完成”“CPU 可以复用资源”三种条件。它们可能由不同同步对象表达。不要把交换链图像索引、帧资源槽和完成标记混成一个变量。

## Vulkan 原生 的对象、参数与执行推导

acquire semaphore 让提交等待图像可用，render-finished semaphore 供 present 等待，fence 让 CPU 复用上一代资源。独立实验验证 signaled→reset→submit→complete；实际窗口路径只在成功 acquire 后 reset fence，避免错误分支留下永远不会完成的等待。

## 对照 DirectX 12：相同问题的另一种分工

back buffer 在 PRESENT/COMMON 与渲染用途间转换，Present 与 queue fence 各司其职。DX12 fence 是单调计数，不能套用 Vulkan binary fence 的 reset 模型；实验连续推进完成值并检查。

## 关键参数与可检查不变量

| 检查项 | Vulkan 表达 | DX12 表达 | 不变量 / 证据 |
| --- | --- | --- | --- |
| 图像可用 | acquire semaphore | 当前 back buffer 与呈现规则 | 不能写仍由呈现系统使用的图像 |
| 渲染完成 | present 等待 render-finished | 队列/呈现及 fence 规则 | CPU 完成标记不机械等于呈现消费 |
| 复用模型 | binary fence 可 reset | fence value 单调推进 | 两种接口不一一替换 |

先为每项指出源码中的创建/绑定位置，以及证明它成立的实际日志、字节或图像。数值中的设备限制以查询为准；固定实验尺寸不当作所有设备的上限。

## nvpro_core2 框架对照

Application 负责其呈现链路，但自定义 UBO/离屏目标需要明确复用规则。框架的完成对象与用户资源生命周期要一起检查。

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
| Vulkan 原生 | [vk48_fence_lifecycle](../../lessons/vk48_fence_lifecycle/README.md) |
| Vulkan 原生 | [vk02_swapchain](../../lessons/vk02_swapchain/README.md) |
| DirectX 12 | [dx48_fence_lifecycle](../../../../DirectX12/lessons/dx48_fence_lifecycle/README.md) |
| DirectX 12 | [dx02_swapchain](../../../../DirectX12/lessons/dx02_swapchain/README.md) |

从合集根目录执行；普通图形程序 Esc 退出，设备/计算/诊断程序输出结果后结束。从“开始学习.cmd”进入本页时，表格可直接点击运行。

```powershell
cmake --preset vulkan-native
cmake --build --preset vulkan-native-debug --target vk20_ch_present_sync --parallel 4
.\Vulkan\native\build\bin\Debug\vk20_ch_present_sync.exe
ctest --preset vulkan-native-debug -R "^vk20_ch_present_sync$" --output-on-failure
```

检查三个层次：创建/绑定/提交的接口约束；数据读回或目标画面；资源使用结束后的回收。诊断实验用合法信息或查询证明渠道可用，不故意保留 API 错误。图形 smoke 的像素检查与数值实验的逐项检查不同，具体实现见源码。

## 关键实现：从当前源码定位

来自 [Vulkan/native/chapters/20_present_sync/main.cpp](main.cpp#L29)，第 29—50 行。下列是当前仓库的局部切片，可能在函数中间结束；完整上下文请打开文件，切片不作为单独可编译程序。

```cpp
int runChapter(int argc, char** argv) {
    auto options = labOptions(argc, argv, 48);
    nativeShaders = options.directory / "shaders";
    validationErrors = 0;
    {
        NativeDevice owner(nullptr, false);
        const auto& c = owner.c;
        {
            inspection(c);
        }
    }
    std::cout << "Validation errors: " << validationErrors << '\n';
    return validationErrors ? 1 : 0;
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

解释为何 Vulkan 在 reset 后遇到 OUT_OF_DATE 并直接 continue，可能让下一帧永久等待。

先记录预测，再只改一个因素；保留基线与修改后的输出。不要同时改变 shader、资源范围和同步，这样即使结果变好也无法归因。

### 参考推导与通过标准

该轮没有提交去 signal 已重置 fence。正确顺序应先处理 acquire 的可恢复失败，再对确定会提交的路径重置。

## 常见失败与排查顺序

把 CPU fence 等待当成 present 已消费 binary semaphore 的通用证明，会使信号量复用条件错误。

先看第一条错误和当前执行路径，再看实际数据/绑定，最后看 shader 与画面。需要 GPU 证据时抓取正常交互帧；仅四帧的 smoke 适合自动检查，不适合手动抓帧。

## 自测与本课交付

能否不看源码回答：解释为何 Vulkan 在 reset 后遇到 OUT_OF_DATE 并直接 continue，可能让下一帧永久等待。

提交一份简短记录：输入与假设、唯一修改、预期、实际证据、原因、是否恢复基线。如果只能复述函数名而说不清何时允许访问/释放，先回到本课的数据流。

## 官方阅读位置与当前工程差异

[指定 Vulkan 教程对应位置](https://docs.vulkan.net.cn/tutorial/latest/03_Drawing_a_triangle/03_Drawing/02_Rendering_and_presentation.html)。DX12 路线按同一概念顺序建立映射，其对象与约束以 Microsoft 文档和本后端代码为准。49—54 课包含在基础/compute 之后加入的工程扩展。

[版本与资料说明](../../../../docs/REFERENCES.md) · [调试](../../../../docs/DEBUGGING.md) · [功能完成度](../../../../docs/FEATURE_MATRIX.md)
