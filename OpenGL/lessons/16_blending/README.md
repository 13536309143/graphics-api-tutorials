# 第 16 课：透明混合与排序

[课程首页](../../README.md) · [上一课](../15_model_gltf/README.md) · [下一课](../17_stencil/README.md)

## 目标与运行

```powershell
cd OpenGL
.\scripts\run.ps1 -Lesson 16_blending
```

预期画面：立方体前面有三张半透明圆片，重叠处颜色正确混合。

相机：W/A/S/D 移动，Q/E 升降，按住右键转向，Esc 退出。

## 代码阅读顺序

main.cpp 的 ordered 排序 → GL_BLEND/DepthMask → shaders/surface.frag

从本课 [main.cpp](main.cpp) 进入；公共工具在 [common](../../common)，GLSL 在 [shaders](../../shaders)。先画出每一遍读什么、写什么，再逐个展开辅助类。

## 原理与实现

标准非预乘 Alpha 混合为 C结果=源颜色*源Alpha+目标颜色*(1-源Alpha)。glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA) 设置这两个因子，输出的 RGB 不应再提前乘一次 Alpha。基础颜色 PNG 使用 sRGB 解码，但此入门 surface shader 已进行显示编码，混合不是严格线性工作流；产品渲染应先在线性 HDR FBO 内混合，再统一编码。

先画不透明物体并写深度，再画透明物体。透明阶段保留深度测试，以便被墙挡住；glDepthMask(GL_FALSE) 禁止透明片自己写深度，避免提前阻挡后续片。最后必须恢复为 GL_TRUE。glClear 的深度清除同样受深度写掩码影响，不能在关闭写入后直接开始下一帧清深度。

当前对透明物体中心按到相机的距离从远到近排序。排序后的 vector 保留相同距离的物体，不使用以距离为唯一键的 map，避免同距离元素被覆盖。中心排序是有限近似，交叉透明面或很大的物体可能需要拆面或顺序无关透明方案。

图片边界 alpha=0 的片段用 discard 裁掉，相当于 cutout 的简单形式；真正半透明区域仍依赖混合。透明阶段若关闭深度测试，圆片就会穿过立方体错误显示。

## 修改练习

取消排序并从前后两个方向观察；将 Alpha 改为 1 与 0；尝试预乘 Alpha 并同步修改混合因子。

修改后重新执行 `cmake --build build --config Debug --target 16_blending --parallel`。GLSL、模型、图片会复制到程序旁的 shaders/assets 目录。源文件应修改教程根目录下的版本，运行目录的副本会被下一次构建覆盖。

## 自测与参考答案

DepthMask(false) 会关闭深度测试吗？

不会。它只禁止写深度，测试仍由 GL_DEPTH_TEST 控制。

## 检查与排错

运行 `16_blending.exe --smoke` 可执行确定时间的四帧渲染检查。重要技术开关还可用 `--smoke --variant --capture` 与默认抓图对比。共享工具会检查 GL 错误、非背景像素和图像动态范围，但这不能代替人工核对技术含义。参见 [进阶排错](../../docs/11_进阶排错与性能.md)。


## 参考截图

以下为课程自带素材、固定时间 1 秒时的实际渲染；你的相机与操作改变后画面会不同。

![16_blending](../../docs/previews/16_blending.png)
