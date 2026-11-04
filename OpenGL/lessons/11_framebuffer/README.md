# 第 11 课：离屏渲染与后处理

[课程首页](../../README.md) · [上一课](../10_materials/README.md) · [下一课](../12_instancing/README.md)

## 学完你应该能够

说明“把渲染结果再次当作输入”，运行并修改本课示例。

## 运行与观察

在教程根目录执行：

```powershell
.\scripts\run.ps1 -Lesson 11_framebuffer
```

预期画面：灰度棋盘立方体；按住空格显示彩色原图。

## 原理与代码阅读

默认帧缓冲负责窗口显示，自定义 FBO 允许将图像渲染进纹理。本课 FBO 有 RGB8 颜色纹理和 DEPTH24_STENCIL8 renderbuffer。颜色之后要采样，选择纹理；深度只做测试，选择 renderbuffer。

第一遍绑定 target.fbo、启用深度测试、清除颜色和深度，然后画立方体。第二遍绑定 framebuffer 0，关闭深度测试，在全屏矩形上采样第一遍颜色。灰度公式 dot(rgb, vec3(0.2126,0.7152,0.0722)) 是线性 RGB 常用亮度权重；本课用于理解后处理，并未加入完整 sRGB 管线。

不能一边往某张纹理附件写入，一边在同一 pass 采样它，否则会形成未定义的反馈。若需要多次模糊，采用两张纹理交替读写。FBO 创建或缩放后使用 glCheckFramebufferStatus 检查完整性。

窗口缩放时颜色和深度附件一起重新分配，并使用新宽高；最小化到 0 像素时跳过分配。Framebuffer 类使用 RAII，退出时释放 GPU 对象。

首先阅读 [main.cpp](main.cpp)，再追踪其中调用的 [common/app.hpp](../../common/app.hpp) 和 [GLSL 文件](../../shaders)。不要一次通读所有公共工具：先弄清本课调用路径，再展开对应实现。

## 动手练习

反相输出 1-color；实现左右分屏；在 screen.frag 中加入 3×3 锐化卷积。

每次修改 C++ 或 GLSL 后，从根目录执行 `cmake --build build --config Debug --parallel`。运行同一示例对比修改前后；尝试先预测画面，再修改代码。

## 自测

为什么第二遍要关闭深度测试？

参考答案：全屏后处理不需要场景深度，残留深度状态可能错误遮挡屏幕矩形。

## 出错时优先检查

初始化顺序、当前程序、VAO/纹理绑定、uniform 值与坐标范围。Shader 语法错误会显示编译日志，OpenGL 状态错误会在帧尾输出错误编号。参见 [排错手册](../../docs/06_排错手册.md)。
