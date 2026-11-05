# 第 31 课 · 动态 UBO 与 Root CBV 地址选择 · DirectX 12

[54 课目录](../../COURSE.md) · [对照精讲](../../../docs/compare/05_binding_constants.md) · [上一课](../30_descriptor_instances/README.md) · [下一课](../32_per_draw_constants/README.md)

## 本课目标与先修

学习目标：解释“动态 UBO 与 Root CBV 地址选择”解决的问题，找到实际对象和参数，运行对应实验并用数据/画面证明。先完成前一课，并能说明它交付的对象与完成条件；本课在该依赖上增加新契约。

## 为什么需要这一层

将三份参数放在同一缓冲的对齐槽中，切换访问范围而不复制整份内容。必须同时检查 base offset、dynamic offset、range、对齐和缓冲总容量，不能只看槽编号。

## DirectX 12 的对象、参数与执行推导

同一 UPLOAD buffer 中每槽 256 字节，以 root CBV GPU 地址加 slot*256 选择。没有 Vulkan 式 dynamic descriptor type，但要解决相同的对齐、范围和在途更新问题。

## 对照 Vulkan 原生：相同问题的另一种分工

使用 UNIFORM_BUFFER_DYNAMIC，bind 时传一个动态偏移。三个槽使用设备要求的 stride，descriptor range 保持 16 字节；shader 读取每槽不同 factor/bias，逐项验证结果。

## 关键参数与可检查不变量

| 检查项 | Vulkan 表达 | DX12 表达 | 不变量 / 证据 |
| --- | --- | --- | --- |
| 常量槽 | 3 个对齐范围 | 3 个 256-byte 范围 | 每槽保存 16-byte 参数 |
| 参数取值 | (2,3) / (5,11) / (7,13) | 相同参数 | 输出 i*factor+bias 验证实际选择 |
| 最终地址 | base + dynamic offset | GPU base + slot*256 | offset+range 不越过缓冲 |

先为每项指出源码中的创建/绑定位置，以及证明它成立的实际日志、字节或图像。数值中的设备限制以查询为准；固定实验尺寸不当作所有设备的上限。

## nvpro_core2 框架对照

框架路径仍可使用原生 dynamic descriptor；不要假定 allocator 已自动把任意应用偏移变成合法对齐。

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
| DirectX 12 | [dx15_dynamic_binding](../../lessons/dx15_dynamic_binding/README.md) |
| Vulkan 原生 | [vk15_dynamic_binding](../../../Vulkan/native/lessons/vk15_dynamic_binding/README.md) |

从合集根目录执行；普通图形程序 Esc 退出，设备/计算/诊断程序输出结果后结束。从“开始学习.cmd”进入本页时，表格可直接点击运行。

```powershell
cmake --preset dx12
cmake --build --preset dx12-debug --target dx31_ch_dynamic_offsets --parallel 4
.\DirectX12\build\bin\Debug\dx31_ch_dynamic_offsets.exe
ctest --preset dx12-debug -R "^dx31_ch_dynamic_offsets$" --output-on-failure
```

检查三个层次：创建/绑定/提交的接口约束；数据读回或目标画面；资源使用结束后的回收。诊断实验用合法信息或查询证明渠道可用，不故意保留 API 错误。图形 smoke 的像素检查与数值实验的逐项检查不同，具体实现见源码。

## 关键实现：从当前源码定位

来自 [DirectX12/chapters/31_dynamic_offsets/main.cpp](main.cpp#L136)，第 136—172 行。下列是当前仓库的局部切片，可能在函数中间结束；完整上下文请打开文件，切片不作为单独可编译程序。

```cpp
int runChapter(int argc, char** argv) {
    auto options = labOptions(argc, argv, 15);
    DxDevice c(options.warp);
    {
        ComputeKernel kernel(c, options);
        uint32_t mode = ((1));
        uint32_t count = (128);
        for (uint32_t slot = 0; slot < ((3u)); ++slot) {
            uint32_t factor = (3), bias = (7);
            kernel.execute(mode, count, factor, bias, slot);
            auto result = readBuffer(kernel.readback.Get(), mode == 2 ? (count + 63) / 64 : count);
            if (mode == 2) {
                auto sum = std::accumulate(result.begin(), result.end(), 0u);
                require(sum == count * (count + 1) / 2, "Shared reduction result mismatch");
            } else {
                if (mode == 1) {
                    factor = slot == 0 ? 2 : slot == 1 ? 5 : 7;
                    bias = slot == 0 ? 3 : slot == 1 ? 11 : 13;
                }
                for (uint32_t i = 0; i < count; ++i)
                    require(result[i] == i * factor + bias, "Compute/root-binding mismatch");
            }
        }
        std::cout << "Verified experiment " << 15 << ", items=" << count
                  << ", constant stride=256\n";
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

把三个槽的参数互换而不改变 shader，预测结果应如何变化。

先记录预测，再只改一个因素；保留基线与修改后的输出。不要同时改变 shader、资源范围和同步，这样即使结果变好也无法归因。

### 参考推导与通过标准

输出应随绑定范围改变。若 CPU 内容变了而输出仍来自旧槽，检查实际 bind offset/address，而不是重新编译 shader。

## 常见失败与排查顺序

range=WHOLE_SIZE 后再添加动态偏移可能越过缓冲末端。对齐正确也不代表范围自动合法。

先看第一条错误和当前执行路径，再看实际数据/绑定，最后看 shader 与画面。需要 GPU 证据时抓取正常交互帧；仅四帧的 smoke 适合自动检查，不适合手动抓帧。

## 自测与本课交付

能否不看源码回答：把三个槽的参数互换而不改变 shader，预测结果应如何变化。

提交一份简短记录：输入与假设、唯一修改、预期、实际证据、原因、是否恢复基线。如果只能复述函数名而说不清何时允许访问/释放，先回到本课的数据流。

## 官方阅读位置与当前工程差异

[指定 Vulkan 教程对应位置](https://docs.vulkan.net.cn/tutorial/latest/05_Uniform_buffers/01_Descriptor_pool_and_sets.html)。DX12 路线按同一概念顺序建立映射，其对象与约束以 Microsoft 文档和本后端代码为准。49—54 课包含在基础/compute 之后加入的工程扩展。

[版本与资料说明](../../../docs/REFERENCES.md) · [调试](../../../docs/DEBUGGING.md) · [功能完成度](../../../docs/FEATURE_MATRIX.md)
