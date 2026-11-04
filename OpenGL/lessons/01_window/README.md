# 第 01 课：窗口与渲染循环

[课程首页](../../README.md) · [上一课](../../README.md) · [下一课](../02_triangle/README.md)

## 学完你应该能够

说明“窗口为什么不是 OpenGL 本身”，运行并修改本课示例。

## 运行与观察

在合集根目录执行（包含四条路线 preset 的目录）：

```powershell
cmake --preset opengl
cmake --build --preset opengl-debug --target 01_window --parallel 4
.\OpenGL\build\bin\Debug\01_window.exe
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

每次修改 C++ 或 GLSL 后，从根目录执行 `cmake --build --preset opengl-debug --target 01_window --parallel 4`。运行同一示例对比修改前后；尝试先预测画面，再修改代码。

## 自测

若只调用 glClearColor 而不 glClear，会发生什么？

参考答案：glClearColor 改的是状态，不会更新像素。应在每帧清除颜色缓冲。

## 出错时优先检查

初始化顺序、当前程序、VAO/纹理绑定、uniform 值与坐标范围。Shader 语法错误会显示编译日志，OpenGL 状态错误会在帧尾输出错误编号。参见 [排错手册](../../docs/06_排错手册.md)。


## 深入学习：数据、接口与执行

窗口负责事件，context 负责 OpenGL 状态。沿 App 构造、begin、clear、end 四个位置阅读，区分初始化一次与每帧重复的动作。glClearColor 设置后持续生效，glClear 才真正清除当前目标。

配套精讲：[三 API 对照 · 窗口、设备与队列](../../../docs/compare/01_context_device.md)。先读概念与参数选择，再返回本课实现核对，不必一次展开所有公共类。

### 本课的真实源码切片

来自 [OpenGL/lessons/01_window/main.cpp](main.cpp#L5)，第 5—15 行；这是现有实现的局部摘录，末尾可能处于函数中间，不是可单独编译的完整程序。

```cpp
int main(int argc, char** argv) try {
    App app(argc, argv, "01_window - OpenGL Course");
    // 渲染循环 = 处理事件 → 设置视口 → 清屏 → 交换前后缓冲。
    while (app.begin()) {
        app.clear();
        app.end();
    }
    return 0;
} catch (const std::exception& e) {
    return failure(e);
}
```

### 同主题跨 API 导航

以下是同主题参考；场景、功能完成度和版本基线可能不同，具体边界见对照章节。

| 路线 | 可运行参照 / 当前范围 |
| --- | --- |
| OpenGL | [01_window](README.md) |
| Vulkan 原生 | [vk01_instance_device](../../../Vulkan/native/lessons/vk01_instance_device/README.md) |
| Vulkan / nvpro_core2 | [nv01_context](../../../Vulkan/nvpro/lessons/nv01_context/README.md) |
| DirectX 12 | [dx01_device](../../../DirectX12/lessons/dx01_device/README.md) |

### 先预测，再验证

只改 glClearColor 而不调用 glClear，会保证下一帧背景变化吗？

参考推导：不会。前者只更新状态，后者才发出清除工作；同样，处理键盘事件也不等于提交了一帧图像。

在一次实验中只改一个因素，记录预测、实际画面/日志和原因。检查已有 smoke 是否仍通过；图形调试器里再核对本次 draw 的实际输入/输出，不能仅凭进程正常退出下结论。
