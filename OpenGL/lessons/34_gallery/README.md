# 第 34 课：综合实战：PBR 模型展厅

[课程首页](../../README.md) · [上一课](../33_point_shadows/README.md) · [下一课](../../docs/12_后续专题与边界.md)

## 目标与运行

```powershell
cd OpenGL
.\scripts\run.ps1 -Lesson 34_gallery
```

预期画面：机器人模型和两列 PBR 球置于 HDR 环境中，带 Bloom；空格关闭 Bloom。

相机：W/A/S/D 移动，Q/E 升降，按住右键转向，Esc 退出。

## 代码阅读顺序

Model + Environment → scene HDR → bright extraction → ping/pong blur → tone output

从本课 [main.cpp](main.cpp) 进入；公共工具在 [common](../../common)，GLSL 在 [shaders](../../shaders)。先画出每一遍读什么、写什么，再逐个展开辅助类。

## 原理与实现

前面的独立示例可以分别看懂技术，综合场景则检验它们的数据约定是否一致。本课用实际 glTF 模型、材质贴图、PBR、完整 IBL、自由相机、HDR、亮度提取和 Bloom 串成一条可运行管线。

初始化阶段只做一次资产导入与 IBL 预计算。每帧先更新相机与目标尺寸，在线性 RGBA16F 场景目标中绘制模型与球，天空同样输出线性 HDR。与第 25 课不同，这里 PBR 的 displayEncode=0，不能在对象 shader 内提前 tone map/Gamma。

接着在独立 FBO 从 HDR 场景提取亮区，交替横纵模糊，再与原场景相加，最后只在 tone pass 统一曝光映射和编码。这与第 22 课的 MRT 亮区输出是两种等效组织：此处复用单颜色输出的 PBR 程序，所以增加一次全屏亮区提取。

本课不把所有高级效果硬塞到同一 shader：它没有同时集成 SSAO、透明物体、点光阴影和骨骼动画。它是一条已实现的完整模型展示管线；后续练习要求你按清晰的 pass 约定继续合并，而不是靠遗留绑定状态碰巧显示。

每一遍都明确输入纹理、目标附件、viewport、深度状态和色彩空间。若画面突然过亮，先检查是否重复 Gamma，再检查曝光和光源，不要先通过改 baseColor 掩盖流程错误。

## 修改练习

将机器人换成自己的 glTF；接入骨骼动画；把粒子加入 HDR 场景；选择一种阴影与 PBR 的直接光可见性结合。

修改后重新执行 `cmake --build build --config Debug --target 34_gallery --parallel`。GLSL、模型、图片会复制到程序旁的 shaders/assets 目录。源文件应修改教程根目录下的版本，运行目录的副本会被下一次构建覆盖。

## 自测与参考答案

为什么单课 PBR 看起来正常，接入 HDR 展厅却可能变灰或过亮？

最常见原因是对象阶段已经做了显示映射，又在最终 pass 重复映射/编码，必须统一整条管线的输出约定。

## 检查与排错

运行 `34_gallery.exe --smoke` 可执行确定时间的四帧渲染检查。重要技术开关还可用 `--smoke --variant --capture` 与默认抓图对比。共享工具会检查 GL 错误、非背景像素和图像动态范围，但这不能代替人工核对技术含义。参见 [进阶排错](../../docs/11_进阶排错与性能.md)。


## 参考截图

以下为课程自带素材、固定时间 1 秒时的实际渲染；你的相机与操作改变后画面会不同。

![34_gallery](../../docs/previews/34_gallery.png)

[查看按住空格时的对照图](../../docs/previews/34_gallery_off.png)
