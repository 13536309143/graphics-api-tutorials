# 第 04 课：Shader 与 Uniform 动画

[课程首页](../../README.md) · [上一课](../03_indexed/README.md) · [下一课](../05_texture/README.md)

## 学完你应该能够

说明“Uniform 与 GLSL 数据流”，运行并修改本课示例。

## 运行与观察

在教程根目录执行：

```powershell
.\scripts\run.ps1 -Lesson 04_uniform
```

预期画面：矩形周期性明暗变化。

## 原理与代码阅读

顶点属性在顶点之间不同，uniform 则在一次绘制期间保持不变，由 CPU 设置。shader.use() 调用 glUseProgram，随后 shader.set 才更新这个程序的 uniform。设置错误的当前程序可能得到 GL_INVALID_OPERATION。

此课把时间作为 CPU 输入，计算 0.55+0.45*sin(2*t)，再更新片段着色器的 pulse。范围为 [0.1,1]，作用于插值颜色。时间由秒计量，2 是角速度，周期为 π 秒；与每帧固定增加数值不同，这种动画不会因为帧率不同而改变速度。

GLSL 顶点阶段 out 与片段阶段 in 必须类型和名称匹配。uniform 由编译器优化掉时，glGetUniformLocation 返回 -1，设置它不会报错。本教程共享多个 shader，因此一些未用属性或 uniform 被优化是正常现象；自己的必需参数应检查名称、大小写和是否真正参与输出。

修改 GLSL 后重新构建，会把 shaders 目录复制到 exe 旁；程序从 exe 所在目录查找文件，切换 PowerShell 工作目录也能运行。

首先阅读 [main.cpp](main.cpp)，再追踪其中调用的 [common/app.hpp](../../common/app.hpp) 和 [GLSL 文件](../../shaders)。不要一次通读所有公共工具：先弄清本课调用路径，再展开对应实现。

## 动手练习

把频率改为原来的两倍；加入 uniform vec3 tint 实现整体调色。

每次修改 C++ 或 GLSL 后，从根目录执行 `cmake --build build --config Debug --parallel`。运行同一示例对比修改前后；尝试先预测画面，再修改代码。

## 自测

为什么调用 set 之前要 use？

参考答案：glUniform 修改当前激活程序的 uniform，不会根据 location 自动切换程序。

## 出错时优先检查

初始化顺序、当前程序、VAO/纹理绑定、uniform 值与坐标范围。Shader 语法错误会显示编译日志，OpenGL 状态错误会在帧尾输出错误编号。参见 [排错手册](../../docs/06_排错手册.md)。
