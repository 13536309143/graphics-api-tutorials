# 第 26 课：延迟渲染与 G-buffer

[课程首页](../../README.md) · [上一课](../25_ibl/README.md) · [下一课](../27_ssao/README.md)

## 目标与运行

```powershell
cd OpenGL
.\scripts\run.ps1 -Lesson 26_deferred
```

预期画面：一组球和地面被 32 盏彩灯照亮；按 2 查看法线。

相机：W/A/S/D 移动，Q/E 升降，按住右键转向，Esc 退出。

## 代码阅读顺序

geometry pass → gbuffer.frag → deferred.frag 全屏 lighting pass

从本课 [main.cpp](main.cpp) 进入；公共工具在 [common](../../common)，GLSL 在 [shaders](../../shaders)。先画出每一遍读什么、写什么，再逐个展开辅助类。

## 原理与实现

前向渲染对每个物体直接计算光照；延迟渲染先记录可见表面的属性，再在屏幕空间统一计算光。G-buffer 保存观察空间 position、normal、albedo/roughness。每个片段通过 layout(location=...) 写入不同颜色附件，glDrawBuffers 开启 MRT。

本课用三个 RGBA16F 附件统一实现，便于阅读；真实产品会为不同属性选择更紧凑格式、用深度重建位置、对法线编码。960×640 下三张 RGBA16F 颜色图约 14 MiB，尚不含深度和其他目标，带宽很快成为成本。

位置和法线都在观察空间，所以光源也乘 view 后再计算。背景位置的 alpha 清为 0，无几何时跳过光照。相机移动之后若只更新灯却没更新 G-buffer，空间将不一致。

32 盏灯在全屏逐一计算只是在教学上展示结构，并非高效的 clustered/tiled 实现。延迟渲染也不能天然处理所有半透明层，需要混合前向透明 pass；与 MSAA 配合通常更复杂。

## 修改练习

增加/减少光源数量；显示位置和 albedo 附件；用深度重建位置替代 position 附件。

修改后重新执行 `cmake --build build --config Debug --target 26_deferred --parallel`。GLSL、模型、图片会复制到程序旁的 shaders/assets 目录。源文件应修改教程根目录下的版本，运行目录的副本会被下一次构建覆盖。

## 自测与参考答案

延迟渲染为什么不意味着光源越多成本越低？

本例每个可见像素仍循环全部光源，成本随像素数和灯数增长，只是减少不可见表面的重复着色。

## 检查与排错

运行 `26_deferred.exe --smoke` 可执行确定时间的四帧渲染检查。重要技术开关还可用 `--smoke --variant --capture` 与默认抓图对比。共享工具会检查 GL 错误、非背景像素和图像动态范围，但这不能代替人工核对技术含义。参见 [进阶排错](../../docs/11_进阶排错与性能.md)。


## 参考截图

以下为课程自带素材、固定时间 1 秒时的实际渲染；你的相机与操作改变后画面会不同。

![26_deferred](../../docs/previews/26_deferred.png)
