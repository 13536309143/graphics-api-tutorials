# 第 33 课：点光源全方向阴影

[课程首页](../../README.md) · [上一课](../32_particles/README.md) · [下一课](../34_gallery/README.md)

## 目标与运行

```powershell
cd OpenGL
.\scripts\run.ps1 -Lesson 33_point_shadows
```

预期画面：移动点光源照亮四个立方体，地面阴影向各方向延伸；空格关闭阴影。

相机：W/A/S/D 移动，Q/E 升降，按住右键转向，Esc 退出。

## 代码阅读顺序

point_depth.vert/frag → 六个 cubemap 面 → point_shadow.frag

从本课 [main.cpp](main.cpp) 进入；公共工具在 [common](../../common)，GLSL 在 [shaders](../../shaders)。先画出每一遍读什么、写什么，再逐个展开辅助类。

## 原理与实现

第 13 课只有一个有限正交视锥，不能覆盖点光源全方向。本课从光源位置向六个轴向各渲染一次 90° 透视深度，形成 depth cubemap。使用六次 draw pass，未采用几何 shader 分层输出，方便逐面检查。

片段深度写成 length(worldPos-light)/farPlane，因此 cubemap 存的是线性径向距离，而不是默认透视 z。正常着色时用 worldPos-light 的方向采样，乘 farPlane 恢复距离，再与当前点距离比较。若写的是透视 z，却按径向距离读取，结果不会正确。

每面使用正确的 up 向量，并切换 GL_DEPTH_ATTACHMENT。颜色输出关闭；每个方向都清深度。恢复默认 FBO 和窗口 viewport 后再画正常场景。点光源运动时六面都需要更新，成本明显高于一张方向光阴影。

本课用 512² 深度、最近采样和固定 bias=.05，故边缘有锯齿，偏置也可能产生 acne/peter panning。扩展时可在球面方向上多次采样进行 PCF、使用角度与距离相关偏置。更大的分辨率并非唯一质量因素。

## 修改练习

依次显示六面深度；增大分辨率；实现多个方向的 PCF；调整光源高度。

修改后重新执行 `cmake --build build --config Debug --target 33_point_shadows --parallel`。GLSL、模型、图片会复制到程序旁的 shaders/assets 目录。源文件应修改教程根目录下的版本，运行目录的副本会被下一次构建覆盖。

## 自测与参考答案

为什么点阴影用方向采样而不需为每个片段手选 cubemap 面？

Cubemap 采样器根据方向的最大绝对分量自动选面并计算面内坐标。

## 检查与排错

运行 `33_point_shadows.exe --smoke` 可执行确定时间的四帧渲染检查。重要技术开关还可用 `--smoke --variant --capture` 与默认抓图对比。共享工具会检查 GL 错误、非背景像素和图像动态范围，但这不能代替人工核对技术含义。参见 [进阶排错](../../docs/11_进阶排错与性能.md)。


## 参考截图

以下为课程自带素材、固定时间 1 秒时的实际渲染；你的相机与操作改变后画面会不同。

![33_point_shadows](../../docs/previews/33_point_shadows.png)

[查看按住空格时的对照图](../../docs/previews/33_point_shadows_off.png)
