# 第 03 课 · 应用结构、RAII 与 GPU 使用期 · Vulkan 原生

[54 课目录](../../COURSE.md) · [对照精讲](../../../../docs/compare/09_commands_frames.md) · [上一课](../02_environment/README.md) · [下一课](../04_instance_factory/README.md)

## 本课目标与先修

学习目标：解释“应用结构、RAII 与 GPU 使用期”解决的问题，找到实际对象和参数，运行对应实验并用数据/画面证明。先完成前一课，并能说明它交付的对象与完成条件；本课在该依赖上增加新契约。

## 为什么需要这一层

CPU 对象的作用域与 GPU 工作的使用期是两条时间线。析构能够释放 C++ 拥有的资源，却不能自动证明最后一次提交已完成。先定义拥有关系，再把 GPU 完成标记作为回收前置条件。

本实验重复创建上传、设备和读回资源，每代提交完成并核对结果后再退出作用域。这是容易验证的安全基线，不是高吞吐延迟回收器。工程化时应把“本代资源 + 最后使用完成值”放入回收队列，等待完成值越过后回收。

## Vulkan 原生 的对象、参数与执行推导

Buffer 销毁后才能释放它绑定的 memory；依赖 Device 的对象先于 Device 结束。command pool、descriptor pool 和 image view 也各有使用期。vkDeviceWaitIdle 可作为教学退出边界，但逐资源 fence/timeline 可以更精确地回收。

## 对照 DirectX 12：相同问题的另一种分工

ComPtr 只控制 COM 引用计数；最后一个引用释放时若 GPU 还在用资源，仍然错误。UPLOAD 的 CPU 字节也不能在 copy 未完成时覆盖。fence 完成值同时保护 resource、descriptor 范围和 command allocator 的回收。

## 关键参数与可检查不变量

| 检查项 | Vulkan 表达 | DX12 表达 | 不变量 / 证据 |
| --- | --- | --- | --- |
| CPU 所有权 | RAII 包装句柄 | ComPtr 引用计数 | CPU 析构不等于 GPU 完成 |
| 回收条件 | 最后一次提交的 fence/timeline | 最后使用的 fence value | 完成后才回收 upload、descriptor、命令存储 |
| 本实验范围 | 四代串行闭环 | 四代串行闭环 | 不是已实现异步垃圾回收器 |

先为每项指出源码中的创建/绑定位置，以及证明它成立的实际日志、字节或图像。数值中的设备限制以查询为准；固定实验尺寸不当作所有设备的上限。

## nvpro_core2 框架对照

ResourceAllocator 的 destroyBuffer 仍要求调用方确保无在途引用。onDetach 应先等待 GPU，再释放依赖 allocator 的对象，最后 deinit allocator。

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
| Vulkan 原生 | [vk31_retire_resources](../../lessons/vk31_retire_resources/README.md) |
| DirectX 12 | [dx31_retire_resources](../../../../DirectX12/lessons/dx31_retire_resources/README.md) |

从合集根目录执行；普通图形程序 Esc 退出，设备/计算/诊断程序输出结果后结束。从“开始学习.cmd”进入本页时，表格可直接点击运行。

```powershell
cmake --preset vulkan-native
cmake --build --preset vulkan-native-debug --target vk03_ch_lifetime --parallel 4
.\Vulkan\native\build\bin\Debug\vk03_ch_lifetime.exe
ctest --preset vulkan-native-debug -R "^vk03_ch_lifetime$" --output-on-failure
```

检查三个层次：创建/绑定/提交的接口约束；数据读回或目标画面；资源使用结束后的回收。诊断实验用合法信息或查询证明渠道可用，不故意保留 API 错误。图形 smoke 的像素检查与数值实验的逐项检查不同，具体实现见源码。

## 关键实现：从当前源码定位

来自 [Vulkan/native/chapters/03_lifetime/main.cpp](main.cpp#L79)，第 79—102 行。下列是当前仓库的局部切片，可能在函数中间结束；完整上下文请打开文件，切片不作为单独可编译程序。

```cpp
int runChapter(int argc, char** argv) {
    auto options = labOptions(argc, argv, 31);
    nativeShaders = options.directory / "shaders";
    validationErrors = 0;
    {
        NativeDevice owner(nullptr, false);
        const auto& c = owner.c;
        {
            for (int i = 0; i < 4; ++i)
                stagingCopy(c);
            std::cout << "Retired 4 upload/device/readback generations after completion\n";
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

将临时 upload 的回收点标到时间图上；指出“Execute/Submit 返回”之后还缺哪条证据。

先记录预测，再只改一个因素；保留基线与修改后的输出。不要同时改变 shader、资源范围和同步，这样即使结果变好也无法归因。

### 参考推导与通过标准

需要该资源最后一次使用所对应的 GPU 完成条件。提交返回仅说明工作已交给执行系统，不说明 copy 或 draw 已读完。

## 常见失败与排查顺序

调试器里 C++ 指针不为空不能证明 GPU 句柄仍有效；某个驱动偶尔未崩溃也不能证明回收顺序合法。

先看第一条错误和当前执行路径，再看实际数据/绑定，最后看 shader 与画面。需要 GPU 证据时抓取正常交互帧；仅四帧的 smoke 适合自动检查，不适合手动抓帧。

## 自测与本课交付

能否不看源码回答：将临时 upload 的回收点标到时间图上；指出“Execute/Submit 返回”之后还缺哪条证据。

提交一份简短记录：输入与假设、唯一修改、预期、实际证据、原因、是否恢复基线。如果只能复述函数名而说不清何时允许访问/释放，先回到本课的数据流。

## 官方阅读位置与当前工程差异

[指定 Vulkan 教程对应位置](https://docs.vulkan.net.cn/tutorial/latest/03_Drawing_a_triangle/00_Setup/00_Base_code.html)。DX12 路线按同一概念顺序建立映射，其对象与约束以 Microsoft 文档和本后端代码为准。49—54 课包含在基础/compute 之后加入的工程扩展。

[版本与资料说明](../../../../docs/REFERENCES.md) · [调试](../../../../docs/DEBUGGING.md) · [功能完成度](../../../../docs/FEATURE_MATRIX.md)
