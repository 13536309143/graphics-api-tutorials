# 第 12 课：实例化绘制

[课程首页](../../README.md) · [上一课](../11_framebuffer/README.md) · [下一课](../13_shadows/README.md)

## 学完你应该能够

说明“一次提交很多相同网格”，运行并修改本课示例。

## 运行与观察

在教程根目录执行：

```powershell
.\scripts\run.ps1 -Lesson 12_instancing
```

预期画面：900 个彩色小立方体形成起伏方阵，可移动相机。

## 原理与代码阅读

CPU 向驱动发送 draw call 有成本。900 个立方体分别 draw 要提交 900 次，实例化将相同网格与不同的每实例数据结合，一次 glDrawArraysInstanced 完成 900 份几何。

位置/法线属于每顶点属性，offset 属于每实例属性。glVertexAttribDivisor(3,1) 告诉 GPU：location=3 每画完一个实例才读取下一个值。如果 divisor 为 0，就会按顶点推进读取，导致数据错乱甚至越界。

本例实例缓冲是 glm::vec3 的数组，stride 必须用 sizeof(glm::vec3)，避免将潜在对齐填充误认为紧密的三个 float。900 个 offset 在初始化时上传，波浪动画在顶点 shader 里由 time 和 offset 算出，无需每帧重新传全部位置。

实例化减少提交成本，但不会自动减少顶点数、遮挡与片段着色开销。相机看不到的实例仍会参与处理，更多对象时需要视锥剔除和 LOD。若每实例矩阵为 mat4，需要连续占用四个属性位置，并分别设置 divisor。

首先阅读 [main.cpp](main.cpp)，再追踪其中调用的 [common/app.hpp](../../common/app.hpp) 和 [GLSL 文件](../../shaders)。不要一次通读所有公共工具：先弄清本课调用路径，再展开对应实现。

## 动手练习

把方阵增大到 100×100；使用 gl_InstanceID 计算颜色；对比逐对象 draw 的 CPU 成本。

每次修改 C++ 或 GLSL 后，从根目录执行 `cmake --build build --config Debug --parallel`。运行同一示例对比修改前后；尝试先预测画面，再修改代码。

## 自测

实例化后为什么不一定更快？

参考答案：若原先瓶颈在像素填充、复杂 shader 或过多顶点，减少 draw call 不能直接消除这些开销。

## 出错时优先检查

初始化顺序、当前程序、VAO/纹理绑定、uniform 值与坐标范围。Shader 语法错误会显示编译日志，OpenGL 状态错误会在帧尾输出错误编号。参见 [排错手册](../../docs/06_排错手册.md)。
