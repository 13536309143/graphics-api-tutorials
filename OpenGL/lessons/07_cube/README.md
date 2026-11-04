# 第 07 课：三维坐标与深度测试

[课程首页](../../README.md) · [上一课](../06_transform/README.md) · [下一课](../08_camera/README.md)

## 学完你应该能够

说明“MVP 与深度缓冲”，运行并修改本课示例。

## 运行与观察

在教程根目录执行：

```powershell
.\scripts\run.ps1 -Lesson 07_cube
```

预期画面：有透视感的棋盘立方体旋转，前面正确遮挡后面。

## 原理与代码阅读

完整坐标链是局部空间 → model → 世界空间 → view → 观察空间 → projection → 裁剪空间 → 透视除法 → NDC → viewport → 像素。GLSL 表达为 projection*view*model*vec4(pos,1)。

model 把立方体放到世界中；glm::lookAt 定义相机视图，它相当于对世界施加相机姿态的逆变换。glm::perspective 使用垂直视场角、宽高比、near 和 far。默认 OpenGL 约定中相机朝观察空间 -Z 看，近远面参数本身仍然是正距离。

深度测试 glEnable(GL_DEPTH_TEST) 让更近的片段遮住更远的片段。每帧必须清除 GL_DEPTH_BUFFER_BIT，否则上一帧的深度会干扰本帧。深度不是线性分布，near 过小会浪费远处精度，引发 z-fighting。

cube() 输出每面独立的法线和 UV，36 个顶点构成 12 个三角形。虽然空间位置只有 8 个角，但不同面的法线/UV 不同，不能直接以 8 个完整顶点表达硬边立方体。

首先阅读 [main.cpp](main.cpp)，再追踪其中调用的 [common/app.hpp](../../common/app.hpp) 和 [GLSL 文件](../../shaders)。不要一次通读所有公共工具：先弄清本课调用路径，再展开对应实现。

## 动手练习

暂时禁用深度测试看穿插；修改 FOV 为 70 度；调整 near/far。

每次修改 C++ 或 GLSL 后，从根目录执行 `cmake --build build --config Debug --parallel`。运行同一示例对比修改前后；尝试先预测画面，再修改代码。

## 自测

为什么改变窗口大小要重新算 projection？

参考答案：projection 的 aspect 必须匹配实际 framebuffer，否则场景会被横向或纵向拉伸。

## 出错时优先检查

初始化顺序、当前程序、VAO/纹理绑定、uniform 值与坐标范围。Shader 语法错误会显示编译日志，OpenGL 状态错误会在帧尾输出错误编号。参见 [排错手册](../../docs/06_排错手册.md)。
