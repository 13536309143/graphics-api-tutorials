# 第 18 课 · 命令池、Allocator 与安全重置 · Vulkan 原生

[54 课目录](../../COURSE.md) · [对照精讲](../../../../docs/compare/09_commands_frames.md) · [上一课](../17_targets/README.md) · [下一课](../19_record_submit/README.md)

## 本课目标与先修

学习目标：解释“命令池、Allocator 与安全重置”解决的问题，找到实际对象和参数，运行对应实验并用数据/画面证明。先完成前一课，并能说明它交付的对象与完成条件；本课在该依赖上增加新契约。

## 为什么需要这一层

录制接口对象与保存命令的存储需要分别理解。重置可能覆盖 GPU 仍在读取的数据，因此要先建立完成条件。本课用四代不同结果确认重用没有读到旧命令。

## Vulkan 原生 的对象、参数与执行推导

从与队列族匹配的 pool 分配 command buffer，完成后 reset pool，再录制新一代 fill。pool reset 与单独 buffer reset 的 flag/生命周期规则不同；本实验选择整池重置以展示最清楚的拥有关系。

## 对照 DirectX 12：相同问题的另一种分工

CommandAllocator 保存录制存储，CommandList 提供录制接口。只有相关 GPU 使用结束后才能 reset allocator；command list 对象自身的 Reset 条件不应被误当成存储已安全复用。

## 关键参数与可检查不变量

| 检查项 | Vulkan 表达 | DX12 表达 | 不变量 / 证据 |
| --- | --- | --- | --- |
| 命令使用期 | pool/buffer 最后一次执行完成 | allocator 关联执行完成 | Reset 的安全条件来自 GPU 进度 |
| 重置粒度 | 整 pool 或允许单 buffer reset | allocator 和 list 分开 reset | 对象与存储的规则分别核对 |
| 四代验证 | 填充 41/42/43/44 | 复制 41/42/43/44 | 每代核对，避免只看最后一代 |

先为每项指出源码中的创建/绑定位置，以及证明它成立的实际日志、字节或图像。数值中的设备限制以查询为准；固定实验尺寸不当作所有设备的上限。

## nvpro_core2 框架对照

Application 管理自己的命令，临时上传命令有另外的提交/等待接口。不要在自定义代码中随意 reset 框架正在使用的 pool/list。

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
| Vulkan 原生 | [vk44_command_reuse](../../lessons/vk44_command_reuse/README.md) |
| DirectX 12 | [dx44_command_reuse](../../../../DirectX12/lessons/dx44_command_reuse/README.md) |

从合集根目录执行；普通图形程序 Esc 退出，设备/计算/诊断程序输出结果后结束。从“开始学习.cmd”进入本页时，表格可直接点击运行。

```powershell
cmake --preset vulkan-native
cmake --build --preset vulkan-native-debug --target vk18_ch_command_storage --parallel 4
.\Vulkan\native\build\bin\Debug\vk18_ch_command_storage.exe
ctest --preset vulkan-native-debug -R "^vk18_ch_command_storage$" --output-on-failure
```

检查三个层次：创建/绑定/提交的接口约束；数据读回或目标画面；资源使用结束后的回收。诊断实验用合法信息或查询证明渠道可用，不故意保留 API 错误。图形 smoke 的像素检查与数值实验的逐项检查不同，具体实现见源码。

## 关键实现：从当前源码定位

来自 [Vulkan/native/chapters/18_command_storage/main.cpp](main.cpp#L92)，第 92—113 行。下列是当前仓库的局部切片，可能在函数中间结束；完整上下文请打开文件，切片不作为单独可编译程序。

```cpp
int runChapter(int argc, char** argv) {
    auto options = labOptions(argc, argv, 44);
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

四次录制分别写 41、42、43、44，怎样验证重置与执行链路？

先记录预测，再只改一个因素；保留基线与修改后的输出。不要同时改变 shader、资源范围和同步，这样即使结果变好也无法归因。

### 参考推导与通过标准

每代都等待其完成并核对全部结果，再允许下一次覆盖命令存储。仅最终值正确不能证明中间代次没有生命周期错误。

## 常见失败与排查顺序

以固定 sleep 替代 fence 并不能证明安全，换设备、驱动或调度后可能立即暴露竞态。

先看第一条错误和当前执行路径，再看实际数据/绑定，最后看 shader 与画面。需要 GPU 证据时抓取正常交互帧；仅四帧的 smoke 适合自动检查，不适合手动抓帧。

## 自测与本课交付

能否不看源码回答：四次录制分别写 41、42、43、44，怎样验证重置与执行链路？

提交一份简短记录：输入与假设、唯一修改、预期、实际证据、原因、是否恢复基线。如果只能复述函数名而说不清何时允许访问/释放，先回到本课的数据流。

## 官方阅读位置与当前工程差异

[指定 Vulkan 教程对应位置](https://docs.vulkan.net.cn/tutorial/latest/03_Drawing_a_triangle/03_Drawing/01_Command_buffers.html)。DX12 路线按同一概念顺序建立映射，其对象与约束以 Microsoft 文档和本后端代码为准。49—54 课包含在基础/compute 之后加入的工程扩展。

[版本与资料说明](../../../../docs/REFERENCES.md) · [调试](../../../../docs/DEBUGGING.md) · [功能完成度](../../../../docs/FEATURE_MATRIX.md)
