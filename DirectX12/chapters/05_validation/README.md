# 第 05 课 · 验证层、调试队列与诊断证据 · DirectX 12

[54 课目录](../../COURSE.md) · [对照精讲](../../../docs/compare/14_debug_performance.md) · [上一课](../04_instance_factory/README.md) · [下一课](../06_physical_device/README.md)

## 本课目标与先修

学习目标：解释“验证层、调试队列与诊断证据”解决的问题，找到实际对象和参数，运行对应实验并用数据/画面证明。先完成前一课，并能说明它交付的对象与完成条件；本课在该依赖上增加新契约。

## 为什么需要这一层

API 验证负责检查使用约束，应用断言负责检查业务结果。画面正常但有同步错误仍不算通过；验证零错误也不证明法线空间或材质公式正确。本课用合法的信息消息验证诊断链路，不以故意破坏 API 当常规验收。

## DirectX 12 的对象、参数与执行推导

debug layer 要在 Device 创建前启用，InfoQueue 检查 ERROR/CORRUPTION。实验通过 AddApplicationMessage 验证信息通道并为 queue/list/allocator SetName。原始 BeginEvent 是工具层诊断接口；课程不直接伪造 PIX 事件载荷，嵌套 PIX 标记需接入其正式 helper。

## 对照 Vulkan 原生：相同问题的另一种分工

枚举 layer 后创建 debug messenger，并按 severity/type 分类。实验额外注册 INFO 回调，提交应用信息并确认回调收到。外部 loader manifest 诊断与应用 VUID 分别记录。GPU 区域标签通过 debug-utils entry points 录制。

## 关键参数与可检查不变量

| 检查项 | Vulkan 表达 | DX12 表达 | 不变量 / 证据 |
| --- | --- | --- | --- |
| 诊断创建时机 | Instance 创建时接 callback | Device 前启用 debug layer | 早期创建错误也能被观察 |
| 合法测试消息 | INFO + GENERAL | AddApplicationMessage(INFO) | 信息投递不等于发生 API 错误 |
| GPU 调试命名 | debug-utils region | SetName；PIX helper 为后续 | 不直接伪造工具事件载荷 |

先为每项指出源码中的创建/绑定位置，以及证明它成立的实际日志、字节或图像。数值中的设备限制以查询为准；固定实验尺寸不当作所有设备的上限。

## nvpro_core2 框架对照

Context 可启用 validation，但自定义资源的错误仍需由应用回调和日志检查。抓 nvapp 帧时区分场景 pass 与最后 UI pass。

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
| DirectX 12 | [dx38_diagnostic_delivery](../../lessons/dx38_diagnostic_delivery/README.md) |
| DirectX 12 | [dx33_debug_labels](../../lessons/dx33_debug_labels/README.md) |
| Vulkan 原生 | [vk38_diagnostic_delivery](../../../Vulkan/native/lessons/vk38_diagnostic_delivery/README.md) |
| Vulkan 原生 | [vk33_debug_labels](../../../Vulkan/native/lessons/vk33_debug_labels/README.md) |

从合集根目录执行；普通图形程序 Esc 退出，设备/计算/诊断程序输出结果后结束。从“开始学习.cmd”进入本页时，表格可直接点击运行。

```powershell
cmake --preset dx12
cmake --build --preset dx12-debug --target dx05_ch_validation --parallel 4
.\DirectX12\build\bin\Debug\dx05_ch_validation.exe
ctest --preset dx12-debug -R "^dx05_ch_validation$" --output-on-failure
```

检查三个层次：创建/绑定/提交的接口约束；数据读回或目标画面；资源使用结束后的回收。诊断实验用合法信息或查询证明渠道可用，不故意保留 API 错误。图形 smoke 的像素检查与数值实验的逐项检查不同，具体实现见源码。

## 关键实现：从当前源码定位

来自 [DirectX12/chapters/05_validation/main.cpp](main.cpp#L28)，第 28—44 行。下列是当前仓库的局部切片，可能在函数中间结束；完整上下文请打开文件，切片不作为单独可编译程序。

```cpp
int runChapter(int argc, char** argv) {
    auto options = labOptions(argc, argv, 38);
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

注入一条 INFO 应用消息，确认它会进入日志/回调而不会增加 API 错误计数。

先记录预测，再只改一个因素；保留基线与修改后的输出。不要同时改变 shader、资源范围和同步，这样即使结果变好也无法归因。

### 参考推导与通过标准

消息严重度与错误计数策略独立。合法的应用信息用于测试诊断渠道，不应冒充 validation error 或通过清空错误队列让检查成功。

## 常见失败与排查顺序

先解决第一条根因。一个无效对象导致的后续数十条报错，通常不需要同时修改数十处代码。

先看第一条错误和当前执行路径，再看实际数据/绑定，最后看 shader 与画面。需要 GPU 证据时抓取正常交互帧；仅四帧的 smoke 适合自动检查，不适合手动抓帧。

## 自测与本课交付

能否不看源码回答：注入一条 INFO 应用消息，确认它会进入日志/回调而不会增加 API 错误计数。

提交一份简短记录：输入与假设、唯一修改、预期、实际证据、原因、是否恢复基线。如果只能复述函数名而说不清何时允许访问/释放，先回到本课的数据流。

## 官方阅读位置与当前工程差异

[指定 Vulkan 教程对应位置](https://docs.vulkan.net.cn/tutorial/latest/03_Drawing_a_triangle/00_Setup/02_Validation_layers.html)。DX12 路线按同一概念顺序建立映射，其对象与约束以 Microsoft 文档和本后端代码为准。49—54 课包含在基础/compute 之后加入的工程扩展。

[版本与资料说明](../../../docs/REFERENCES.md) · [调试](../../../docs/DEBUGGING.md) · [功能完成度](../../../docs/FEATURE_MATRIX.md)
