# 第 19 课：法线贴图与 TBN

[课程首页](../../README.md) · [上一课](../18_cubemap/README.md) · [下一课](../20_parallax/README.md)

## 目标与运行

```powershell
cd OpenGL
.\scripts\run.ps1 -Lesson 19_normal_mapping
```

预期画面：平面上的橙色纹理产生细小明暗凹凸；按住空格关闭法线贴图。

相机：W/A/S/D 移动，Q/E 升降，按住右键转向，Esc 退出。

## 代码阅读顺序

shaders/surface.frag 的 tangentFrame → normalMap 解码

从本课 [main.cpp](main.cpp) 进入；公共工具在 [common](../../common)，GLSL 在 [shaders](../../shaders)。先画出每一遍读什么、写什么，再逐个展开辅助类。

## 原理与实现

法线贴图不改变位置和轮廓，只替换光照使用的表面法线。图片 RGB 存切线空间方向，采样后乘 2 再减 1。中性法线接近 (0.5,0.5,1)，表示朝表面外侧。它是方向数据，必须按线性纹理读取，不能用 sRGB 解码。

TBN 的三列分别是 tangent、bitangent、normal，把切线空间方向转到世界空间。此课通过 dFdx/dFdy 的位置和 UV 梯度计算切线，并正交化 T。B 的符号根据原始 bitangent 与 cross(N,T) 的点积恢复，不能只用屏幕 UV 行列式的符号，否则换观察方向时可能翻转。

UV 退化时导数无法形成有效基，代码给出垂直 N 的备用基；它只能避免数值崩溃，不能恢复错误 UV 的正确外观。更通用的产品模型可以导入预计算切线，保留 tangent.w 的手性，并保证与烘焙工具的切线基一致。

GL 约定的法线贴图通常把 +Y 编到绿色增加方向，其他工具可能使用相反约定。凹凸颠倒时先检查绿色通道，而不是随意反转整个法线。基底方向与纹理内容必须匹配。

## 修改练习

把法线图换成纯中性蓝；反转绿色通道；让平面绕 Y 旋转并观察凹凸是否稳定。

修改后重新执行 `cmake --build build --config Debug --target 19_normal_mapping --parallel`。GLSL、模型、图片会复制到程序旁的 shaders/assets 目录。源文件应修改教程根目录下的版本，运行目录的副本会被下一次构建覆盖。

## 自测与参考答案

法线贴图能让侧面轮廓变成锯齿吗？

不能，它没有增加几何或改变顶点位置。

## 检查与排错

运行 `19_normal_mapping.exe --smoke` 可执行确定时间的四帧渲染检查。重要技术开关还可用 `--smoke --variant --capture` 与默认抓图对比。共享工具会检查 GL 错误、非背景像素和图像动态范围，但这不能代替人工核对技术含义。参见 [进阶排错](../../docs/11_进阶排错与性能.md)。


## 参考截图

以下为课程自带素材、固定时间 1 秒时的实际渲染；你的相机与操作改变后画面会不同。

![19_normal_mapping](../../docs/previews/19_normal_mapping.png)

[查看按住空格时的对照图](../../docs/previews/19_normal_mapping_off.png)
