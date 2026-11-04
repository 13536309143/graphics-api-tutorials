# 第 01 课：窗口与渲染循环

[课程首页](../../README.md) · [上一课](../../README.md) · [下一课](../02_triangle/README.md)

## 学完你应该能够

说明“窗口为什么不是 OpenGL 本身”，运行并修改本课示例。

## 运行与观察

在教程根目录执行：

```powershell
.\scripts\run.ps1 -Lesson 01_window
```

预期画面：深蓝色窗口，拖动大小仍能正常清屏，Esc 退出。

## 原理与代码阅读

OpenGL 定义 GPU 绘图接口，不负责创建操作系统窗口。GLFW 创建窗口和 OpenGL 上下文、处理键鼠；GLAD 在上下文创建成功后加载函数地址；GLM 提供向量和矩阵计算。它们的职责不同，不能相互替代。

上下文记录当前绑定的缓冲、纹理和渲染状态。没有 current context 时调用 OpenGL 属于错误的使用方式。初始化顺序必须是 glfwInit → 指定 3.3 Core → glfwCreateWindow → glfwMakeContextCurrent → gladLoadGL。GLAD 2 使用 glad/gl.h 与 gladLoadGL，不要混用 GLAD 1 的 glad/glad.h 和 gladLoadGLLoader。

双缓冲让 GPU 先画到后台图像，再通过 glfwSwapBuffers 呈现。glClearColor 只设置清屏颜色，glClear 才执行清除。窗口尺寸是屏幕逻辑单位，framebuffer 尺寸才是实际像素；高 DPI 下二者可能不同，必须用 glfwGetFramebufferSize 设置 glViewport。

打开 common/app.hpp 对照构造函数和 begin/end。第 01 课 main.cpp 保留最小渲染循环，不含绘制对象。App 的析构发生在最后，保证 GPU 资源先删除、窗口后销毁。

首先阅读 [main.cpp](main.cpp)，再追踪其中调用的 [common/app.hpp](../../common/app.hpp) 和 [GLSL 文件](../../shaders)。不要一次通读所有公共工具：先弄清本课调用路径，再展开对应实现。

## 动手练习

把清屏颜色改成深红色；再用 sin(time) 让背景缓慢变色。

每次修改 C++ 或 GLSL 后，从根目录执行 `cmake --build build --config Debug --parallel`。运行同一示例对比修改前后；尝试先预测画面，再修改代码。

## 自测

若只调用 glClearColor 而不 glClear，会发生什么？

参考答案：glClearColor 改的是状态，不会更新像素。应在每帧清除颜色缓冲。

## 出错时优先检查

初始化顺序、当前程序、VAO/纹理绑定、uniform 值与坐标范围。Shader 语法错误会显示编译日志，OpenGL 状态错误会在帧尾输出错误编号。参见 [排错手册](../../docs/06_排错手册.md)。
