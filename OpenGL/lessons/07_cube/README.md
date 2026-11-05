# 第 07 课：三维坐标与深度测试

## 本课独立工程与关键注释

[main.cpp](main.cpp) · [本地辅助实现](support/) · [本课着色器](shaders/) · [CMakeLists.txt](CMakeLists.txt)

本目录拥有完整课程源码、资源与依赖配方。关键步骤含中文注释，不链接其他课的课程实现。可将整个目录复制到其他位置，用以下命令单独构建和检查；第三方依赖首次配置需要联网或提供缓存。

```powershell
cmake -S . -B build -A x64
cmake --build build --config Debug --parallel 4
ctest --test-dir build -C Debug --output-on-failure
```

[课程首页](../../README.md) · [上一课](../06_transform/README.md) · [下一课](../08_camera/README.md)

## 学完你应该能够

说明“MVP 与深度缓冲”，运行并修改本课示例。

## 运行与观察

在合集根目录执行（包含四条路线 preset 的目录）：

```powershell
cmake --preset opengl
cmake --build --preset opengl-debug --target 07_cube --parallel 4
.\OpenGL\build\bin\Debug\07_cube.exe
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

每次修改 C++ 或 GLSL 后，从根目录执行 `cmake --build --preset opengl-debug --target 07_cube --parallel 4`。运行同一示例对比修改前后；尝试先预测画面，再修改代码。

## 自测

为什么改变窗口大小要重新算 projection？

参考答案：projection 的 aspect 必须匹配实际 framebuffer，否则场景会被横向或纵向拉伸。

## 出错时优先检查

初始化顺序、当前程序、VAO/纹理绑定、uniform 值与坐标范围。Shader 语法错误会显示编译日志，OpenGL 状态错误会在帧尾输出错误编号。参见 [排错手册](../../docs/06_排错手册.md)。


## 深入学习：数据、接口与执行

本课同时引入透视与深度测试。三维顶点变到 clip space 后还需透视除法；深度缓冲保存最近可见片段，提交顺序不再独自决定遮挡。

配套精讲：[三 API 对照 · 目标、深度与 MSAA](../../../docs/compare/07_targets_depth_msaa.md)。先读概念与参数选择，再返回本课实现核对，不必一次展开所有公共类。

### 本课的真实源码切片

来自 [OpenGL/lessons/07_cube/main.cpp](main.cpp#L7)，第 7—32 行；这是现有实现的局部摘录，末尾可能处于函数中间，不是可单独编译的完整程序。

```cpp
int main(int argc, char** argv) try {
    // 先创建上下文；它必须比 shader、纹理和网格活得更久。
    App app(argc, argv, "07_cube - OpenGL Course");
    // shader 路径指向本课自己的文件；属性、uniform 与下方 C++ 设置对应。
    Shader shader(app, "basic.vert", "texture.frag");
    // CPU 顶点数据上传后，由 VAO 保存属性布局；布局必须匹配顶点 shader。
    Mesh mesh(cube());
    Texture texture;
    Camera camera;
    glEnable(GL_DEPTH_TEST);
    // 每帧处理输入并绘制；GPU 对象在退出循环后按 C++ 作用域逆序释放。
    while (app.begin()) {
        app.clear();
        shader.use();
        texture.bind();
        shader.set("tex", 0);
        auto model = glm::rotate(glm::mat4(1), app.time() * .7f, glm::vec3(.5f, 1, .2f));
        auto view = glm::lookAt(glm::vec3(2, 1.5f, 3), glm::vec3(0), glm::vec3(0, 1, 0));
        auto proj = glm::perspective(glm::radians(45.f), app.aspect(), .1f, 100.f);
        shader.set("mvp", proj * view * model);
        mesh.draw();
        app.end();
    }
    return 0;
} catch (const std::exception& e) {
    return failure(e);
```

### 同主题跨 API 导航

以下是同主题参考；场景、功能完成度和版本基线可能不同，具体边界见对照章节。

| 路线 | 可运行参照 / 当前范围 |
| --- | --- |
| OpenGL | [07_cube](README.md) |
| Vulkan 原生 | [vk10_offscreen](../../../Vulkan/native/lessons/vk10_offscreen/README.md) |
| Vulkan / nvpro_core2 | [nv02_application](../../../Vulkan/nvpro/lessons/nv02_application/README.md) |
| DirectX 12 | [dx10_offscreen](../../../DirectX12/lessons/dx10_offscreen/README.md) |

### 先预测，再验证

只清颜色而不清上一帧深度，旋转时可能怎样？

参考推导：旧深度会拒绝新帧本应可见的片段，形成残留遮挡。颜色和深度的清除是两项独立工作。

在一次实验中只改一个因素，记录预测、实际画面/日志和原因。检查已有 smoke 是否仍通过；图形调试器里再核对本次 draw 的实际输入/输出，不能仅凭进程正常退出下结论。
