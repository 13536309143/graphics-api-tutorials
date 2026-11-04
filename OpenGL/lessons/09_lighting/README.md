# 第 09 课：Phong 光照

[课程首页](../../README.md) · [上一课](../08_camera/README.md) · [下一课](../10_materials/README.md)

## 学完你应该能够

说明“Phong：从几何到明暗”，运行并修改本课示例。

## 运行与观察

在教程根目录执行：

```powershell
.\scripts\run.ps1 -Lesson 09_lighting
```

预期画面：三个不同高光锐度的立方体，光照随时间改变。

## 原理与代码阅读

本课使用环境光 ambient、漫反射 diffuse、镜面反射 specular。环境项提供简化的基础亮度；漫反射强度为 max(dot(N,L),0)，N/L 必须归一化；镜面项用 reflect(-L,N) 与视线 V 的夹角，指数 shininess 控制高光集中程度。

worldPos、normal、eye 和 light 必须处于同一坐标空间。此例全部在世界空间计算。模型经过非均匀缩放时，法线不能直接用 model 变换，而应使用 transpose(inverse(mat3(model)))；随后在片段阶段再次 normalize，因为插值会改变长度。

光源绕场景运动，三个立方体分别使用 shininess=8、32、128。指数越高，高光越小越锐利，不代表物体整体更亮。光从背面照来时漫反射为 0，同时关闭该面的镜面项，避免背面漏高光。

Phong 是便于入门的经验模型，环境项并非真实全局照明。本例默认直接输出线性计算结果到普通颜色缓冲，用于观察公式；线性工作流与 sRGB/Gamma 的正确处理见进阶路线。

首先阅读 [main.cpp](main.cpp)，再追踪其中调用的 [common/app.hpp](../../common/app.hpp) 和 [GLSL 文件](../../shaders)。不要一次通读所有公共工具：先弄清本课调用路径，再展开对应实现。

## 动手练习

改光源高度；关闭环境项；对模型做非均匀缩放对比法线变换。

每次修改 C++ 或 GLSL 后，从根目录执行 `cmake --build build --config Debug --parallel`。运行同一示例对比修改前后；尝试先预测画面，再修改代码。

## 自测

相机移动时漫反射和镜面反射分别如何变化？

参考答案：固定物体与光源时漫反射不依赖视线；镜面项包含 V，因此随相机变化。

## 出错时优先检查

初始化顺序、当前程序、VAO/纹理绑定、uniform 值与坐标范围。Shader 语法错误会显示编译日志，OpenGL 状态错误会在帧尾输出错误编号。参见 [排错手册](../../docs/06_排错手册.md)。
