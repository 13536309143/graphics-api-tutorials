# 第 20 课：视差遮蔽映射

[课程首页](../../README.md) · [上一课](../19_normal_mapping/README.md) · [下一课](../21_hdr/README.md)

## 目标与运行

```powershell
cd OpenGL
.\scripts\run.ps1 -Lesson 20_parallax
```

预期画面：平面纹理有视角相关的深度感，空格关闭 POM 和法线贴图。

相机：W/A/S/D 移动，Q/E 升降，按住右键转向，Esc 退出。

## 代码阅读顺序

surface.frag 的 parallax → height.png → main.cpp 的 heightScale

从本课 [main.cpp](main.cpp) 进入；公共工具在 [common](../../common)，GLSL 在 [shaders](../../shaders)。先画出每一遍读什么、写什么，再逐个展开辅助类。

## 原理与实现

视差遮蔽映射 POM 沿切线空间视线穿过高度场，寻找第一处交点，用该点偏移后的 UV 采样表面。它改变纹理取样位置，仍不增加真实几何；轮廓、投影和碰撞不会自动改变。

本例高度图白色表示高处，采样深度写成 1-height。视线转到切线空间后，偏移与 V.xy/V.z 有关；V.z 太小会放大数值，所以限制分母并按视角选择 12–40 层。掠射角需要更多层，但本例上限以可读和性能为主。

步进到采样深度之后，使用当前点与前一点做线性细化，减轻层状跳变。UV 超出 [0,1] 时 discard，避免重复边缘；如果想做平铺材质，应明确保留重复并保证高度图边界连续。heightScale 控制近似深度，过大可能产生穿帮。

示例的 normal 与 height 都是教学生成素材，不是同一真实高度场严格求导的完整材质包。用于独立观察两种技术；产品素材应保持它们一致。POM 的自阴影与真实阴影贴图联动未在本课实现。

## 修改练习

将相机移到斜侧面；把高度缩放从 0.02 调到 0.12；把层数固定为 8 比较误差。

修改后重新执行 `cmake --build build --config Debug --target 20_parallax --parallel`。GLSL、模型、图片会复制到程序旁的 shaders/assets 目录。源文件应修改教程根目录下的版本，运行目录的副本会被下一次构建覆盖。

## 自测与参考答案

为何 POM 能看起来有深度却没有改变阴影？

它只改变片段的 UV；阴影深度遍仍然渲染原来的平面几何。

## 检查与排错

运行 `20_parallax.exe --smoke` 可执行确定时间的四帧渲染检查。重要技术开关还可用 `--smoke --variant --capture` 与默认抓图对比。共享工具会检查 GL 错误、非背景像素和图像动态范围，但这不能代替人工核对技术含义。参见 [进阶排错](../../docs/11_进阶排错与性能.md)。


## 参考截图

以下为课程自带素材、固定时间 1 秒时的实际渲染；你的相机与操作改变后画面会不同。

![20_parallax](../../docs/previews/20_parallax.png)

[查看按住空格时的对照图](../../docs/previews/20_parallax_off.png)
