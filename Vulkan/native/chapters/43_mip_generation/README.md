# 第 43 课 · GPU Mip 链生成与逐级验收 · Vulkan 原生

[54 课目录](../../COURSE.md) · [对照精讲](../../../../docs/compare/06_textures_color.md) · [上一课](../42_gltf_nodes/README.md) · [下一课](../44_mip_sampling/README.md)

## 本课目标与先修

学习目标：解释“GPU Mip 链生成与逐级验收”解决的问题，找到实际对象和参数，运行对应实验并用数据/画面证明。先完成前一课，并能说明它交付的对象与完成条件；本课在该依赖上增加新契约。

## 为什么需要这一层

mip 链是预先缩小的图像集合，层数通常到 1×1。资源级数、上传内容、每级状态、view 和 sampler LOD 必须一起更新。本课不仅创建多个级别，还逐 texel 读回并与 CPU 参考比较。

## Vulkan 原生 的对象、参数与执行推导

查询格式的 blit/filter 支持，上传 level 0，逐级 transition 和 linear blit，再把所有级别转成 shader-read。每次只读取已经完成的前一级，最后复制每级结果用于校验。数值参考使用线性 UNORM。

## 对照 DirectX 12：相同问题的另一种分工

GPU 计算使用 SRV 指前一级、UAV 指后一级，8×8 工作组平均四个 texel。每级 descriptor 与 subresource state 分开管理，最后转可采样。sRGB 材质链另用正确线性平均的 CPU 路径，未把编码值简单平均冒充正确颜色 mip。

## 关键参数与可检查不变量

| 检查项 | Vulkan 表达 | DX12 表达 | 不变量 / 证据 |
| --- | --- | --- | --- |
| 默认基图 | 64x64 线性 checker | 相同输入 | 7 级到 1x1 |
| GPU 生成 | 逐级 linear blit | SRV→UAV 8x8 compute | 每级写后下一层读有依赖 |
| 验收 | 每 texel 对 CPU 参考 | 每 texel 对 CPU 参考 | 允许有限量化差异，不只看末级 |

先为每项指出源码中的创建/绑定位置，以及证明它成立的实际日志、字节或图像。数值中的设备限制以查询为准；固定实验尺寸不当作所有设备的上限。

## nvpro_core2 框架对照

框架可能提供更多图像辅助，但应用必须确定支持格式、子资源范围与生成依赖。native mip helper 的规则可直接用于对照分析。

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
| Vulkan 原生 | [vk18_mip_chain](../../lessons/vk18_mip_chain/README.md) |
| DirectX 12 | [dx18_mip_chain](../../../../DirectX12/lessons/dx18_mip_chain/README.md) |

从合集根目录执行；普通图形程序 Esc 退出，设备/计算/诊断程序输出结果后结束。从“开始学习.cmd”进入本页时，表格可直接点击运行。

```powershell
cmake --preset vulkan-native
cmake --build --preset vulkan-native-debug --target vk43_ch_mip_generation --parallel 4
.\Vulkan\native\build\bin\Debug\vk43_ch_mip_generation.exe
ctest --preset vulkan-native-debug -R "^vk43_ch_mip_generation$" --output-on-failure
```

检查三个层次：创建/绑定/提交的接口约束；数据读回或目标画面；资源使用结束后的回收。诊断实验用合法信息或查询证明渠道可用，不故意保留 API 错误。图形 smoke 的像素检查与数值实验的逐项检查不同，具体实现见源码。

## 关键实现：从当前源码定位

来自 [Vulkan/native/chapters/43_mip_generation/main.cpp](main.cpp#L244)，第 244—266 行。下列是当前仓库的局部切片，可能在函数中间结束；完整上下文请打开文件，切片不作为单独可编译程序。

```cpp
int runChapter(int argc, char** argv) {
    auto options = labOptions(argc, argv, 18);
    nativeShaders = options.directory / "shaders";
    validationErrors = 0;
    {
        NativeDevice owner(nullptr, false);
        const auto& c = owner.c;
        {
            GpuTexture texture(c, Pixels{64, 64, checker()}, true);
            texture.verify();
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

把基图改为棋盘或分区整数颜色，为什么比任意照片更适合作为 mip 数值验收？

先记录预测，再只改一个因素；保留基线与修改后的输出。不要同时改变 shader、资源范围和同步，这样即使结果变好也无法归因。

### 参考推导与通过标准

可手算局部平均，并发现级数、偏移或尾部错误。图像“看着模糊了”不足以证明每一级都正确。

## 常见失败与排查顺序

只设置 sampler 最大 LOD 却没有生成内容，或用全资源错误 before state 覆盖各级不同状态，会产生未初始化读取。

先看第一条错误和当前执行路径，再看实际数据/绑定，最后看 shader 与画面。需要 GPU 证据时抓取正常交互帧；仅四帧的 smoke 适合自动检查，不适合手动抓帧。

## 自测与本课交付

能否不看源码回答：把基图改为棋盘或分区整数颜色，为什么比任意照片更适合作为 mip 数值验收？

提交一份简短记录：输入与假设、唯一修改、预期、实际证据、原因、是否恢复基线。如果只能复述函数名而说不清何时允许访问/释放，先回到本课的数据流。

## 官方阅读位置与当前工程差异

[指定 Vulkan 教程对应位置](https://docs.vulkan.net.cn/tutorial/latest/09_Generating_Mipmaps.html)。DX12 路线按同一概念顺序建立映射，其对象与约束以 Microsoft 文档和本后端代码为准。49—54 课包含在基础/compute 之后加入的工程扩展。

[版本与资料说明](../../../../docs/REFERENCES.md) · [调试](../../../../docs/DEBUGGING.md) · [功能完成度](../../../../docs/FEATURE_MATRIX.md)
