# 第 05 课：纹理与采样

[课程首页](../../README.md) · [上一课](../04_uniform/README.md) · [下一课](../06_transform/README.md)

## 学完你应该能够

说明“纹理就是可采样的数据”，运行并修改本课示例。

## 运行与观察

在教程根目录执行：

```powershell
.\scripts\run.ps1 -Lesson 05_texture
```

预期画面：矩形显示橙蓝相间的棋盘纹理。

## 原理与代码阅读

纹理坐标 UV 通常覆盖 [0,1]。顶点着色器传出 UV，光栅化插值，片段着色器用 texture(tex,uv) 取样。sampler2D 中设置的是纹理单元序号 0，不是纹理对象 id，也不是 GL_TEXTURE0 常量。

Texture 类在 CPU 生成 64×64 的 RGB 棋盘图，glTexImage2D 上传到 GPU，因此此项目不依赖图片下载或图片解码库。图像的一行可能不满足默认 4 字节对齐；通用 RGB 上传可先设 GL_UNPACK_ALIGNMENT=1，上传后恢复。本例宽度恰好兼容 4 字节，也保留规范做法。

GL_TEXTURE_WRAP_S/T 控制越界 UV 的行为；GL_REPEAT 让图案重复。MIN_FILTER 使用 GL_LINEAR_MIPMAP_LINEAR，缩小时结合 mipmap，减轻闪烁。MAG_FILTER 只能使用 GL_NEAREST 或 GL_LINEAR，不能使用 mipmap 过滤选项。没有生成 mipmap 却要求 mipmap 的纹理可能不完整。

此课 texture.bind() 激活单元 0 并绑定纹理，shader.set("tex",0) 让 sampler 读取此单元。外部 PNG/JPEG 导入属于扩展练习：需要解码为像素，明确通道数、行方向和 sRGB 格式。后续 docs/08_进阶路线.md 给出接入步骤。

首先阅读 [main.cpp](main.cpp)，再追踪其中调用的 [common/app.hpp](../../common/app.hpp) 和 [GLSL 文件](../../shaders)。不要一次通读所有公共工具：先弄清本课调用路径，再展开对应实现。

## 动手练习

把 quad() 的 UV 从 1 改为 4，看重复；将过滤改成 GL_NEAREST 看像素边界。

每次修改 C++ 或 GLSL 后，从根目录执行 `cmake --build build --config Debug --parallel`。运行同一示例对比修改前后；尝试先预测画面，再修改代码。

## 自测

纹理对象 id=7，绑定单元 0 时 sampler 应设多少？

参考答案：设 0。对象编号与纹理单元编号属于两个不同概念。

## 出错时优先检查

初始化顺序、当前程序、VAO/纹理绑定、uniform 值与坐标范围。Shader 语法错误会显示编译日志，OpenGL 状态错误会在帧尾输出错误编号。参见 [排错手册](../../docs/06_排错手册.md)。
