# 第 30 课：几何着色器

## 本课独立工程与关键注释

[main.cpp](main.cpp) · [本地辅助实现](support/) · [本课着色器](shaders/) · [CMakeLists.txt](CMakeLists.txt)

本目录拥有完整课程源码、资源与依赖配方。关键步骤含中文注释，不链接其他课的课程实现。可将整个目录复制到其他位置，用以下命令单独构建和检查；第三方依赖首次配置需要联网或提供缓存。

```powershell
cmake -S . -B build -A x64
cmake --build build --config Debug --parallel 4
ctest --test-dir build -C Debug --output-on-failure
```

[课程首页](../../README.md) · [上一课](../29_msaa/README.md) · [下一课](../31_skeletal/README.md)

## 目标与运行

```powershell
cmake --preset opengl
cmake --build --preset opengl-debug --target 30_geometry --parallel 4
.\OpenGL\build\bin\Debug\30_geometry.exe
```

预期画面：立方体的各三角形沿面方向周期性向外分离。

相机：W/A/S/D 移动，Q/E 升降，按住右键转向，Esc 退出。

## 代码阅读顺序

explode.vert → explode.geom → surface.frag

从本课 [main.cpp](main.cpp) 进入；公共工具在 [common](../../common)，GLSL 在 [shaders](../../shaders)。先画出每一遍读什么、写什么，再逐个展开辅助类。

## 原理与实现

几何着色器位于顶点之后、光栅化之前，按图元接收数据。本课 layout(triangles) in，每次接收三个顶点；输出 triangle_strip，max_vertices=3，逐个 EmitVertex，再 EndPrimitive。

顶点阶段暂时把局部位置放在 gl_Position。几何阶段根据三点叉乘得到面方向，沿方向平移顶点，再应用 model/view/projection。这样在局部或世界空间计算几何方向，避免错误地在透视裁剪空间直接求法线。

原顶点法线用于光照，面方向用于分离位移，二者用途不同。对于带平滑法线的模型，分离会出现接缝，这是本课故意展示的效果。geometryStage 将几何 shader 加入程序重新链接，并检查链接日志。

几何 shader 能做教学上的法线可视化、简单图元扩展等，但不保证比其他方案快。大量放大图元可能触发严重吞吐瓶颈。粒子 billboard 的常见实现是顶点 shader 加实例化，第 32 课采用该方式。

## 修改练习

把位移改成恒定；输出线段可视化每个面的法线；比较 face normal 与 interpolated normal。

修改后重新执行 `cmake --build --preset opengl-debug --target 30_geometry --parallel 4`。GLSL、模型、图片会复制到程序旁的 shaders/assets 目录。源文件应修改教程根目录下的版本，运行目录的副本会被下一次构建覆盖。

## 自测与参考答案

为什么不直接对 gl_Position.xyz 的透视后数据做世界方向计算？

它们已经不在统一欧氏坐标空间，透视和 w 会扭曲长度与角度。

## 检查与排错

运行 `30_geometry.exe --smoke` 可执行确定时间的四帧渲染检查。重要技术开关还可用 `--smoke --variant --capture` 与默认抓图对比。共享工具会检查 GL 错误、非背景像素和图像动态范围，但这不能代替人工核对技术含义。参见 [进阶排错](../../docs/11_进阶排错与性能.md)。


## 参考截图

以下为课程自带素材、固定时间 1 秒时的实际渲染；你的相机与操作改变后画面会不同。

![30_geometry](../../docs/previews/30_geometry.png)


## 深入学习：数据、接口与执行

geometry shader 按输入三角形运行，计算面方向后逐点 EmitVertex，最后 EndPrimitive。它介于顶点处理与光栅化之间，不是每片元执行的特效。

配套精讲：[三 API 对照 · 三角形、管线与着色器](../../../docs/compare/02_triangle_pipeline.md)。先读概念与参数选择，再返回本课实现核对，不必一次展开所有公共类。

### 本课的真实源码切片

来自 [OpenGL/lessons/30_geometry/main.cpp](main.cpp#L7)，第 7—32 行；这是现有实现的局部摘录，末尾可能处于函数中间，不是可单独编译的完整程序。

```cpp
int main(int argc, char** argv) try {
    // 先创建上下文；它必须比 shader、纹理和网格活得更久。
    App app(argc, argv, "30_geometry - OpenGL Course");
    // shader 路径指向本课自己的文件；属性、uniform 与下方 C++ 设置对应。
    Shader shader(app, "explode.vert", "surface.frag");
    geometryStage(shader, app, "explode.geom");
    // CPU 顶点数据上传后，由 VAO 保存属性布局；布局必须匹配顶点 shader。
    Mesh mesh(cube());
    Camera camera;
    glEnable(GL_DEPTH_TEST);
    // 每帧处理输入并绘制；GPU 对象在退出循环后按 C++ 作用域逆序释放。
    while (app.begin()) {
        camera.update(app);
        app.clear();
        shader.use();
        surfaceDefaults(shader);
        cameraUniforms(shader, app, camera);
        shader.set("time", app.time());
        shader.set("model", glm::rotate(glm::mat4(1), app.time() * .2f, glm::vec3(0, 1, 0)));
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
| OpenGL | [30_geometry](README.md) |
| Vulkan 原生 | [vk03_triangle](../../../Vulkan/native/lessons/vk03_triangle/README.md) |
| Vulkan / nvpro_core2 | [nv03_triangle](../../../Vulkan/nvpro/lessons/nv03_triangle/README.md) |
| DirectX 12 | [dx03_triangle](../../../DirectX12/lessons/dx03_triangle/README.md) |

### 先预测，再验证

想提高细分密度，只增加 max_vertices 就会自动生成新几何吗？

参考推导：不会。声明仅给出上限，还需实际发射顶点；本例也不是 tessellation 管线。

在一次实验中只改一个因素，记录预测、实际画面/日志和原因。检查已有 smoke 是否仍通过；图形调试器里再核对本次 draw 的实际输入/输出，不能仅凭进程正常退出下结论。
