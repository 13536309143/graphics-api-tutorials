# 第 32 课 · Push/Root Constants 的单位与边界 · Vulkan 原生

[54 课目录](../../COURSE.md) · [对照精讲](../../../../docs/compare/05_binding_constants.md) · [上一课](../31_dynamic_offsets/README.md) · [下一课](../33_image_decode/README.md)

## 本课目标与先修

学习目标：解释“Push/Root Constants 的单位与边界”解决的问题，找到实际对象和参数，运行对应实验并用数据/画面证明。先完成前一课，并能说明它交付的对象与完成条件；本课在该依赖上增加新契约。

## 为什么需要这一层

小常量适合频繁、少量的对象参数，而不是无限容量的快捷数据通道。本实验用数值计算验证顺序、大小和更新频率，再回到 model/settings 的绘制数据。

## Vulkan 原生 的对象、参数与执行推导

push 常量使用字节 offset/size，并与 pipeline layout 的阶段范围相容。已有图形 Draw 为 80 字节，计算 Operation 为 16 字节；两个 layout 可以有不同约定，不应混用。

## 对照 DirectX 12：相同问题的另一种分工

root constants 使用 32 位值数量与偏移，对应 HLSL cbuffer。20 DWORD 的 Draw 与 4 DWORD 的 Operation 不同；root signature 总成本也限制接口设计。

## 关键参数与可检查不变量

| 检查项 | Vulkan 表达 | DX12 表达 | 不变量 / 证据 |
| --- | --- | --- | --- |
| 参数单位 | 字节 size/offset | DWORD count/offset | 16 bytes = 4 DWORD |
| 测试公式 | factor=9 / bias=17 | 相同公式 | 全量结果而非 CPU 变量作证 |
| 容量界限 | maxPushConstantsSize | root signature 成本限制 | 大骨骼/场景数组放缓冲 |

先为每项指出源码中的创建/绑定位置，以及证明它成立的实际日志、字节或图像。数值中的设备限制以查询为准；固定实验尺寸不当作所有设备的上限。

## nvpro_core2 框架对照

SceneVk 与 nvpro 使用相同 push 布局；如果换自定义 shader，需要自己维护接口兼容，不由 UI 参数自动生成。

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
| Vulkan 原生 | [vk16_push_constants](../../lessons/vk16_push_constants/README.md) |
| DirectX 12 | [dx16_push_constants](../../../../DirectX12/lessons/dx16_push_constants/README.md) |

从合集根目录执行；普通图形程序 Esc 退出，设备/计算/诊断程序输出结果后结束。从“开始学习.cmd”进入本页时，表格可直接点击运行。

```powershell
cmake --preset vulkan-native
cmake --build --preset vulkan-native-debug --target vk32_ch_per_draw_constants --parallel 4
.\Vulkan\native\build\bin\Debug\vk32_ch_per_draw_constants.exe
ctest --preset vulkan-native-debug -R "^vk32_ch_per_draw_constants$" --output-on-failure
```

检查三个层次：创建/绑定/提交的接口约束；数据读回或目标画面；资源使用结束后的回收。诊断实验用合法信息或查询证明渠道可用，不故意保留 API 错误。图形 smoke 的像素检查与数值实验的逐项检查不同，具体实现见源码。

## 关键实现：从当前源码定位

来自 [Vulkan/native/chapters/32_per_draw_constants/main.cpp](main.cpp#L157)，第 157—199 行。下列是当前仓库的局部切片，可能在函数中间结束；完整上下文请打开文件，切片不作为单独可编译程序。

```cpp
int runChapter(int argc, char** argv) {
    auto options = labOptions(argc, argv, 16);
    nativeShaders = options.directory / "shaders";
    validationErrors = 0;
    {
        NativeDevice owner(nullptr, false);
        const auto& c = owner.c;
        {
            ComputeKernel kernel(c);
            uint32_t mode = ((0));
            uint32_t count = (128);
            for (uint32_t slot = 0; slot < ((1u)); ++slot) {
                uint32_t factor = (9), bias = (17);
                kernel.execute(mode, count, factor, bias, slot);
                auto result = readWords(kernel.output, mode == 2 ? (count + 63) / 64 : count);
                if (mode == 2) {
                    uint32_t total = std::accumulate(result.begin(), result.end(), 0u);
                    require(total == count * (count + 1) / 2, "Shared-memory reduction mismatch");
                } else {
                    if (mode == 1) {
                        factor = slot == 0 ? 2 : slot == 1 ? 5 : 7;
                        bias = slot == 0 ? 3 : slot == 1 ? 11 : 13;
                    }
                    for (uint32_t i = 0; i < count; ++i)
                        require(result[i] == i * factor + bias,
                                "Compute/dynamic-binding result mismatch");
                }
            }
            std::cout << "Verified experiment " << 16 << ", items=" << count
                      << ", uniform stride=" << kernel.stride << '\n';
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

加入第二个对象，仅改变它的 model，说明不需要重建哪些长期对象。

先记录预测，再只改一个因素；保留基线与修改后的输出。不要同时改变 shader、资源范围和同步，这样即使结果变好也无法归因。

### 参考推导与通过标准

静态几何、纹理、layout 和同一兼容 pipeline 可以复用，只在对应 draw 前更新小常量。更改不可动态状态或 shader 才可能要求另一 pipeline。

## 常见失败与排查顺序

字节与 DWORD 单位混淆、更新到错误 root parameter，或只更新结构前半段却读取后半段，都会留下错误数据。

先看第一条错误和当前执行路径，再看实际数据/绑定，最后看 shader 与画面。需要 GPU 证据时抓取正常交互帧；仅四帧的 smoke 适合自动检查，不适合手动抓帧。

## 自测与本课交付

能否不看源码回答：加入第二个对象，仅改变它的 model，说明不需要重建哪些长期对象。

提交一份简短记录：输入与假设、唯一修改、预期、实际证据、原因、是否恢复基线。如果只能复述函数名而说不清何时允许访问/释放，先回到本课的数据流。

## 官方阅读位置与当前工程差异

[指定 Vulkan 教程对应位置](https://docs.vulkan.net.cn/tutorial/latest/05_Uniform_buffers/01_Descriptor_pool_and_sets.html)。DX12 路线按同一概念顺序建立映射，其对象与约束以 Microsoft 文档和本后端代码为准。49—54 课包含在基础/compute 之后加入的工程扩展。

[版本与资料说明](../../../../docs/REFERENCES.md) · [调试](../../../../docs/DEBUGGING.md) · [功能完成度](../../../../docs/FEATURE_MATRIX.md)
