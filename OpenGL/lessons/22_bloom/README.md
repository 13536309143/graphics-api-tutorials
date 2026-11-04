# 第 22 课：Bloom 与高斯模糊

[课程首页](../../README.md) · [上一课](../21_hdr/README.md) · [下一课](../23_pbr/README.md)

## 目标与运行

```powershell
cd OpenGL
.\scripts\run.ps1 -Lesson 22_bloom
```

预期画面：高亮球周围出现柔和光晕；空格关闭 Bloom。

相机：W/A/S/D 移动，Q/E 升降，按住右键转向，Esc 退出。

## 代码阅读顺序

hdr_scene.frag 的 bright 输出 → ping/pong → blur.frag → tone.frag

从本课 [main.cpp](main.cpp) 进入；公共工具在 [common](../../common)，GLSL 在 [shaders](../../shaders)。先画出每一遍读什么、写什么，再逐个展开辅助类。

## 原理与实现

Bloom 模拟强亮区域对周围像素的扩散。场景 pass 用 MRT 同时写正常 HDR 颜色和亮度阈值图，阈值依据线性亮度 dot(rgb,weights)>1，不能先把 HDR 压到 [0,1] 再筛高亮。

二维高斯滤波可分离为横向与纵向。本例使用中心加左右四个采样的对称核，交替执行八次。ping 与 pong 两个 FBO 交替读写，每次输出纹理都不同于输入纹理，避免同一纹理反馈。重复多次会增大有效模糊半径。

把模糊结果以 0.25 权重加回线性 HDR 场景，然后统一 tone mapping 与 Gamma。Bloom 不是让基础颜色更白，也不是在最终 sRGB 图片上直接做透明叠加。

示例用全分辨率贴图方便检查，窗口放大时成本会增加。产品实现通常用降采样/上采样金字塔，或不同 mip 的能量分布；极强亮点还可能需要 soft threshold 和 firefly 抑制。

## 修改练习

修改 bright threshold、合成权重和 pass 数；将 ping/pong 改半分辨率并正确设置 viewport。

修改后重新执行 `cmake --build build --config Debug --target 22_bloom --parallel`。GLSL、模型、图片会复制到程序旁的 shaders/assets 目录。源文件应修改教程根目录下的版本，运行目录的副本会被下一次构建覆盖。

## 自测与参考答案

为什么必须用两张模糊目标？

同一 pass 同时采样并写同一纹理会形成未定义反馈，不能保证读到上一遍结果。

## 检查与排错

运行 `22_bloom.exe --smoke` 可执行确定时间的四帧渲染检查。重要技术开关还可用 `--smoke --variant --capture` 与默认抓图对比。共享工具会检查 GL 错误、非背景像素和图像动态范围，但这不能代替人工核对技术含义。参见 [进阶排错](../../docs/11_进阶排错与性能.md)。


## 参考截图

以下为课程自带素材、固定时间 1 秒时的实际渲染；你的相机与操作改变后画面会不同。

![22_bloom](../../docs/previews/22_bloom.png)

[查看按住空格时的对照图](../../docs/previews/22_bloom_off.png)
