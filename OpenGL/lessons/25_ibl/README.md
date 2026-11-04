# 第 25 课：IBL 与 Split Sum

[课程首页](../../README.md) · [上一课](../24_pbr_textures/README.md) · [下一课](../26_deferred/README.md)

## 目标与运行

```powershell
cd OpenGL
.\scripts\run.ps1 -Lesson 25_ibl
```

预期画面：环境照明下的 35 球矩阵与天空，空格关闭 IBL 作对照。

相机：W/A/S/D 移动，Q/E 升降，按住右键转向，Esc 退出。

## 代码阅读顺序

Environment → equirect/irradiance/prefilter/brdf.frag → pbr.frag 的 useIBL

从本课 [main.cpp](main.cpp) 进入；公共工具在 [common](../../common)，GLSL 在 [shaders](../../shaders)。先画出每一遍读什么、写什么，再逐个展开辅助类。

## 原理与实现

直接光只计算明确的灯，IBL 把环境图当作来自各方向的入射光。它不是给金属球随便采一张模糊图。此课从线性 HDR 文件生成环境 cubemap，再计算三类实际预计算资源。

漫反射 irradiance：对法线半球进行余弦重要性采样，128 个 Hammersley 样本估计辐照度。因为样本 PDF 已含 cosθ/π，结果为 π*平均环境值；着色时再乘 Lambert albedo/π，不能重复漏乘或多乘 π。

镜面 prefilter：对 GGX 半程向量重要性采样，假定 V=N，分别计算五级不同粗糙度的 cubemap。每级的内容是独立 GGX 积分，不是对原图 glGenerateMipmap 后就宣称完成粗糙反射。运行时 roughness*4 选择 LOD。此教学实现源环境固定采样 LOD0，低样本数与小分辨率可能让极亮点有噪声。

BRDF LUT：在 128×128 的 RG16F 纹理中积分 NdotV 和 roughness 对应的分离系数 A/B。运行时环境镜面近似为 prefilteredColor*(F0*A+B)。此处 IBL 几何函数使用 k=r²/2，和直接光的近似参数区分。

这就是 split-sum 近似的两部分：预过滤环境与 BRDF 查表，不是精确求解渲染方程。资源分辨率低、每项只用 128 样本，优先保证学习和启动速度。更高品质需要更多采样、源 mip/PDF 选择、多次散射补偿和更完整的局部探针策略。

## 修改练习

将采样数同时在 hammersley 和循环改为 512；增加预过滤分辨率；只显示 irradiance 或 prefilter 的某级作为调试。

修改后重新执行 `cmake --build build --config Debug --target 25_ibl --parallel`。GLSL、模型、图片会复制到程序旁的 shaders/assets 目录。源文件应修改教程根目录下的版本，运行目录的副本会被下一次构建覆盖。

## 自测与参考答案

普通 mipmap 为什么不能代替 GGX 预过滤？

普通 mipmap 按图像空间过滤，GGX 预过滤按反射分布与方向半球积分，二者权重和几何意义不同。

## 检查与排错

运行 `25_ibl.exe --smoke` 可执行确定时间的四帧渲染检查。重要技术开关还可用 `--smoke --variant --capture` 与默认抓图对比。共享工具会检查 GL 错误、非背景像素和图像动态范围，但这不能代替人工核对技术含义。参见 [进阶排错](../../docs/11_进阶排错与性能.md)。


## 参考截图

以下为课程自带素材、固定时间 1 秒时的实际渲染；你的相机与操作改变后画面会不同。

![25_ibl](../../docs/previews/25_ibl.png)

[查看按住空格时的对照图](../../docs/previews/25_ibl_off.png)
