# 第 04 课：Shader 与 Uniform 动画

## 本课独立工程与关键注释

[main.cpp](main.cpp) · [本地辅助实现](support/) · [本课着色器](shaders/) · [CMakeLists.txt](CMakeLists.txt)

本目录拥有完整课程源码、资源与依赖配方。关键步骤含中文注释，不链接其他课的课程实现。可将整个目录复制到其他位置，用以下命令单独构建和检查；第三方依赖首次配置需要联网或提供缓存。

```powershell
cmake -S . -B build -A x64
cmake --build build --config Debug --parallel 4
ctest --test-dir build -C Debug --output-on-failure
```

[课程首页](../../README.md) · [上一课](../03_indexed/README.md) · [下一课](../05_texture/README.md)

## 学完你应该能够

说明“Uniform 与 GLSL 数据流”，运行并修改本课示例。

## 运行与观察

在合集根目录执行（包含四条路线 preset 的目录）：

```powershell
cmake --preset opengl
cmake --build --preset opengl-debug --target 04_uniform --parallel 4
.\OpenGL\build\bin\Debug\04_uniform.exe
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

每次修改 C++ 或 GLSL 后，从根目录执行 `cmake --build --preset opengl-debug --target 04_uniform --parallel 4`。运行同一示例对比修改前后；尝试先预测画面，再修改代码。

## 自测

为什么调用 set 之前要 use？

参考答案：glUniform 修改当前激活程序的 uniform，不会根据 location 自动切换程序。

## 出错时优先检查

初始化顺序、当前程序、VAO/纹理绑定、uniform 值与坐标范围。Shader 语法错误会显示编译日志，OpenGL 状态错误会在帧尾输出错误编号。参见 [排错手册](../../docs/06_排错手册.md)。


## 深入学习：数据、接口与执行

uniform 更新作用于当前 program，并使用其 location。逐帧变化的数值不必重新编译 shader；uniform 名称与类型必须对应，未使用变量可能被优化掉。

配套精讲：[三 API 对照 · 常量与资源绑定](../../../docs/compare/05_binding_constants.md)。先读概念与参数选择，再返回本课实现核对，不必一次展开所有公共类。

### 本课的真实源码切片

来自 [OpenGL/lessons/04_uniform/main.cpp](main.cpp#L7)，第 7—27 行；这是现有实现的局部摘录，末尾可能处于函数中间，不是可单独编译的完整程序。

```cpp
int main(int argc, char** argv) try {
    // 先创建上下文；它必须比 shader、纹理和网格活得更久。
    App app(argc, argv, "04_uniform - OpenGL Course");
    // shader 路径指向本课自己的文件；属性、uniform 与下方 C++ 设置对应。
    Shader shader(app, "basic.vert", "color.frag");
    // CPU 顶点数据上传后，由 VAO 保存属性布局；布局必须匹配顶点 shader。
    Mesh mesh(quad(), quadIndices());
    // 每帧处理输入并绘制；GPU 对象在退出循环后按 C++ 作用域逆序释放。
    while (app.begin()) {
        app.clear();
        shader.use();
        glm::mat4 mvp(1.f);
        shader.set("mvp", mvp);
        shader.set("pulse", 0.55f + 0.45f * std::sin(app.time() * 2));
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
| OpenGL | [04_uniform](README.md) |
| Vulkan 原生 | [vk05_indexed_uniform](../../../Vulkan/native/lessons/vk05_indexed_uniform/README.md) |
| Vulkan / nvpro_core2 | [nv06_pbr_viewport](../../../Vulkan/nvpro/lessons/nv06_pbr_viewport/README.md) |
| DirectX 12 | [dx05_indexed_cbv](../../../DirectX12/lessons/dx05_indexed_cbv/README.md) |

### 先预测，再验证

让第二个 program 使用同名 uniform，能复用第一个 program 的 location 吗？

参考推导：不能把 location 当成全局编号。分别查询并在正确 program 生效时更新，才能解释各自状态。

在一次实验中只改一个因素，记录预测、实际画面/日志和原因。检查已有 smoke 是否仍通过；图形调试器里再核对本次 draw 的实际输入/输出，不能仅凭进程正常退出下结论。
