# 第 02 课：第一个彩色三角形

[课程首页](../../README.md) · [上一课](../01_window/README.md) · [下一课](../03_indexed/README.md)

## 学完你应该能够

说明“从 CPU 数据到 GPU 像素”，运行并修改本课示例。

## 运行与观察

在合集根目录执行（包含四条路线 preset 的目录）：

```powershell
cmake --preset opengl
cmake --build --preset opengl-debug --target 02_triangle --parallel 4
.\OpenGL\build\bin\Debug\02_triangle.exe
```

预期画面：红、绿、蓝三个顶点之间有平滑渐变的三角形。

## 原理与代码阅读

一组三角形的三个位置先在 C++ 数组里。VBO 保存上传到 GPU 的顶点数据，VAO 保存如何解释这些数据的规则。VAO 不保存顶点数据副本，而是引用 VBO 及其属性布局。

本教程每个顶点有 8 个 float：xyz、rgb、uv。位置 location=0，颜色 location=1，UV location=2；stride 是 8*sizeof(float)，颜色 offset 是 3*sizeof(float)，UV offset 是 6*sizeof(float)。glVertexAttribPointer 记录调用时绑定的 VBO，glEnableVertexAttribArray 开启属性。

顶点着色器为每个顶点计算 gl_Position。此课 mvp 是单位矩阵，所以直接使用位置。除以 w 后进入标准化设备坐标 NDC，可见范围 x/y/z 均为 [-1,1]。光栅化生成覆盖区域的片段，并插值顶点颜色；片段着色器输出 FragColor。

阅读 main.cpp → common/app.hpp 的 Mesh → shaders/basic.vert → shaders/color.frag。glDrawArrays 的 count 是顶点数，不是 float 数；这里是 3。顶点、片段着色器在运行时编译和链接，Shader 类会将日志作为错误输出。

首先阅读 [main.cpp](main.cpp)，再追踪其中调用的 [common/app.hpp](../../common/app.hpp) 和 [GLSL 文件](../../shaders)。不要一次通读所有公共工具：先弄清本课调用路径，再展开对应实现。

## 动手练习

移动顶点；把三个顶点颜色都改为黄色；再尝试画两个三角形。

每次修改 C++ 或 GLSL 后，从根目录执行 `cmake --build --preset opengl-debug --target 02_triangle --parallel 4`。运行同一示例对比修改前后；尝试先预测画面，再修改代码。

## 自测

为什么 CPU 数组改动后，不重新上传看不到变化？

参考答案：glBufferData 已把原始数据复制到 GPU。修改 CPU 数组后需再次 glBufferData 或 glBufferSubData。

## 出错时优先检查

初始化顺序、当前程序、VAO/纹理绑定、uniform 值与坐标范围。Shader 语法错误会显示编译日志，OpenGL 状态错误会在帧尾输出错误编号。参见 [排错手册](../../docs/06_排错手册.md)。


## 深入学习：数据、接口与执行

本课三角形确实来自 VBO，顶点中间三个 float 被当成颜色。VAO 描述如何读取八个 float；单位 mvp 让你先排除相机因素。pulse 是材质控制，不会改变顶点数量。

配套精讲：[三 API 对照 · 三角形、管线与着色器](../../../docs/compare/02_triangle_pipeline.md)。先读概念与参数选择，再返回本课实现核对，不必一次展开所有公共类。

### 本课的真实源码切片

来自 [OpenGL/lessons/02_triangle/main.cpp](main.cpp#L5)，第 5—22 行；这是现有实现的局部摘录，末尾可能处于函数中间，不是可单独编译的完整程序。

```cpp
int main(int argc, char** argv) try {
    App app(argc, argv, "02_triangle - OpenGL Course");
    Shader shader(app, "basic.vert", "color.frag");
    Mesh mesh(
        {-.7f, -.5f, 0, 1, 0, 0, 0, 0, .7f, -.5f, 0, 0, 1, 0, 1, 0, 0, .6f, 0, 0, 0, 1, .5f, 1});
    while (app.begin()) {
        app.clear();
        shader.use();
        glm::mat4 mvp(1.f);
        shader.set("mvp", mvp);
        shader.set("pulse", 1.f);
        mesh.draw();
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
| OpenGL | [02_triangle](README.md) |
| Vulkan 原生 | [vk03_triangle](../../../Vulkan/native/lessons/vk03_triangle/README.md) |
| Vulkan / nvpro_core2 | [nv03_triangle](../../../Vulkan/nvpro/lessons/nv03_triangle/README.md) |
| DirectX 12 | [dx03_triangle](../../../DirectX12/lessons/dx03_triangle/README.md) |

### 先预测，再验证

先固定输出洋红，再只移动一个顶点，怎样判断链路哪部分正常？

参考推导：固定色出现说明管线和目标基本工作；顶点移动正确说明输入与位置变换成立。之后才恢复颜色插值，避免同时查三个问题。

在一次实验中只改一个因素，记录预测、实际画面/日志和原因。检查已有 smoke 是否仍通过；图形调试器里再核对本次 draw 的实际输入/输出，不能仅凭进程正常退出下结论。
