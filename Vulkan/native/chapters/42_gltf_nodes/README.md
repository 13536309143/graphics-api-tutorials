# 第 42 课 · glTF 节点层级、法线与静态姿态 · Vulkan 原生

[54 课目录](../../COURSE.md) · [对照精讲](../../../../docs/compare/11_models_animation.md) · [上一课](../41_obj_models/README.md) · [下一课](../43_mip_generation/README.md)

## 本课目标与先修

学习目标：解释“glTF 节点层级、法线与静态姿态”解决的问题，找到实际对象和参数，运行对应实验并用数据/画面证明。先完成前一课，并能说明它交付的对象与完成条件；本课在该依赖上增加新契约。

## 为什么需要这一层

glTF 将 node、mesh、material、image 和 skin 分开。局部坐标要经父子变换累计到场景空间；同一个 mesh 还可能被多个节点引用。成功解码文件不等于完整重现场景。

## Vulkan 原生 的对象、参数与执行推导

本实验递归 parent*local，变换每个节点引用的网格并合并索引，实际绘制 robot.gltf。normal 使用逆转置，随后 bbox 归一化。本实现展平静态姿态，不是完整材质/skin/动画系统。

## 对照 DirectX 12：相同问题的另一种分工

使用相同 CPU 场景结果，后端差异集中于资源与管线。这里使用教学纹理/光照，不宣称完整还原 glTF 的每个材质扩展、透明模式或动画插值。

## 关键参数与可检查不变量

| 检查项 | Vulkan 表达 | DX12 表达 | 不变量 / 证据 |
| --- | --- | --- | --- |
| 节点组合 | global=parent*local | 相同 CPU 节点算法 | 变换只应用一次 |
| 法线 | 逆转置并归一化 | 相同 CPU 处理 | 非均匀缩放下保持垂直关系 |
| 实现范围 | 静态展平，不含完整 skin | 静态展平，不含完整 skin | 材质/动画扩展另行验收 |

先为每项指出源码中的创建/绑定位置，以及证明它成立的实际日志、字节或图像。数值中的设备限制以查询为准；固定实验尺寸不当作所有设备的上限。

## nvpro_core2 框架对照

可将共享 CPU 层接到 ResourceAllocator/StagingUploader，但还需按 node/material 组织 GPU 对象和 draw。不要直接把 OpenGL ImportedMesh 类换名前缀。

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
| Vulkan 原生 | [vk22_gltf_scene](../../lessons/vk22_gltf_scene/README.md) |
| DirectX 12 | [dx22_gltf_scene](../../../../DirectX12/lessons/dx22_gltf_scene/README.md) |

从合集根目录执行；普通图形程序 Esc 退出，设备/计算/诊断程序输出结果后结束。从“开始学习.cmd”进入本页时，表格可直接点击运行。

```powershell
cmake --preset vulkan-native
cmake --build --preset vulkan-native-debug --target vk42_ch_gltf_nodes --parallel 4
.\Vulkan\native\build\bin\Debug\vk42_ch_gltf_nodes.exe
ctest --preset vulkan-native-debug -R "^vk42_ch_gltf_nodes$" --output-on-failure
```

检查三个层次：创建/绑定/提交的接口约束；数据读回或目标画面；资源使用结束后的回收。诊断实验用合法信息或查询证明渠道可用，不故意保留 API 错误。图形 smoke 的像素检查与数值实验的逐项检查不同，具体实现见源码。

## 关键实现：从当前源码定位

来自 [Vulkan/native/chapters/42_gltf_nodes/main.cpp](main.cpp#L31)，第 31—45 行。下列是当前仓库的局部切片，可能在函数中间结束；完整上下文请打开文件，切片不作为单独可编译程序。

```cpp
int runChapter(int argc, char** argv) {
    auto options = labOptions(argc, argv, 22);
    nativeShaders = options.directory / "shaders";
    {
        return graphicsLab(argc, argv, options);
    }
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

给 glTF 的父节点增加平移，预测所有后代是否一起移动，并说明法线如何处理非均匀缩放。

先记录预测，再只改一个因素；保留基线与修改后的输出。不要同时改变 shader、资源范围和同步，这样即使结果变好也无法归因。

### 参考推导与通过标准

后代 global 包含父变换，因此一起移动；法线使用线性部分的逆转置。单纯把 mesh 放原点会破坏层级关系。

## 常见失败与排查顺序

重复应用 meshGlobal、忽略父变换或把 inverse bind pose 当节点当前姿态，是不同问题，先用无动画静态场景隔离。

先看第一条错误和当前执行路径，再看实际数据/绑定，最后看 shader 与画面。需要 GPU 证据时抓取正常交互帧；仅四帧的 smoke 适合自动检查，不适合手动抓帧。

## 自测与本课交付

能否不看源码回答：给 glTF 的父节点增加平移，预测所有后代是否一起移动，并说明法线如何处理非均匀缩放。

提交一份简短记录：输入与假设、唯一修改、预期、实际证据、原因、是否恢复基线。如果只能复述函数名而说不清何时允许访问/释放，先回到本课的数据流。

## 官方阅读位置与当前工程差异

[指定 Vulkan 教程对应位置](https://docs.vulkan.net.cn/tutorial/latest/08_Loading_models.html)。DX12 路线按同一概念顺序建立映射，其对象与约束以 Microsoft 文档和本后端代码为准。49—54 课包含在基础/compute 之后加入的工程扩展。

[版本与资料说明](../../../../docs/REFERENCES.md) · [调试](../../../../docs/DEBUGGING.md) · [功能完成度](../../../../docs/FEATURE_MATRIX.md)

## 固定输入的实际 GPU 读回

![vk42_ch_gltf_nodes](../../../../docs/previews/vk22_gltf_scene.png)
