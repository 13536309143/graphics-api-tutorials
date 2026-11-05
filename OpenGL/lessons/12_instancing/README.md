# 第 12 课：实例化绘制

## 本课独立工程与关键注释

[main.cpp](main.cpp) · [本地辅助实现](support/) · [本课着色器](shaders/) · [CMakeLists.txt](CMakeLists.txt)

本目录拥有完整课程源码、资源与依赖配方。关键步骤含中文注释，不链接其他课的课程实现。可将整个目录复制到其他位置，用以下命令单独构建和检查；第三方依赖首次配置需要联网或提供缓存。

```powershell
cmake -S . -B build -A x64
cmake --build build --config Debug --parallel 4
ctest --test-dir build -C Debug --output-on-failure
```

[课程首页](../../README.md) · [上一课](../11_framebuffer/README.md) · [下一课](../13_shadows/README.md)

## 学完你应该能够

说明“一次提交很多相同网格”，运行并修改本课示例。

## 运行与观察

在合集根目录执行（包含四条路线 preset 的目录）：

```powershell
cmake --preset opengl
cmake --build --preset opengl-debug --target 12_instancing --parallel 4
.\OpenGL\build\bin\Debug\12_instancing.exe
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

每次修改 C++ 或 GLSL 后，从根目录执行 `cmake --build --preset opengl-debug --target 12_instancing --parallel 4`。运行同一示例对比修改前后；尝试先预测画面，再修改代码。

## 自测

实例化后为什么不一定更快？

参考答案：若原先瓶颈在像素填充、复杂 shader 或过多顶点，减少 draw call 不能直接消除这些开销。

## 出错时优先检查

初始化顺序、当前程序、VAO/纹理绑定、uniform 值与坐标范围。Shader 语法错误会显示编译日志，OpenGL 状态错误会在帧尾输出错误编号。参见 [排错手册](../../docs/06_排错手册.md)。


## 深入学习：数据、接口与执行

实例缓冲的 offset 通过 divisor=1 每实例推进。顶点属性仍每顶点推进，两种频率共同决定读取。一次 draw 的 instance count 与几何顶点数是独立参数。

配套精讲：[三 API 对照 · 顶点、索引与内存](../../../docs/compare/03_geometry_memory.md)。先读概念与参数选择，再返回本课实现核对，不必一次展开所有公共类。

### 本课的真实源码切片

来自 [OpenGL/lessons/12_instancing/main.cpp](main.cpp#L7)，第 7—32 行；这是现有实现的局部摘录，末尾可能处于函数中间，不是可单独编译的完整程序。

```cpp
int main(int argc, char** argv) try {
    // 先创建上下文；它必须比 shader、纹理和网格活得更久。
    App app(argc, argv, "12_instancing - OpenGL Course");
    // shader 路径指向本课自己的文件；属性、uniform 与下方 C++ 设置对应。
    Shader shader(app, "instance.vert", "color.frag");
    // CPU 顶点数据上传后，由 VAO 保存属性布局；布局必须匹配顶点 shader。
    Mesh mesh(cube());
    Camera camera;
    camera.pos = glm::vec3(0, 5, 18);
    camera.pitch = -15;
    std::vector<glm::vec3> offsets;
    for (int z = 0; z < 30; ++z)
        for (int x = 0; x < 30; ++x)
            offsets.emplace_back((x - 15) * .8f, 0, -z * .8f);
    GLuint instances;
    glGenBuffers(1, &instances);
    glBindVertexArray(mesh.vao);
    glBindBuffer(GL_ARRAY_BUFFER, instances);
    glBufferData(GL_ARRAY_BUFFER,
                 GLsizeiptr(offsets.size() * sizeof(glm::vec3)),
                 offsets.data(),
                 GL_STATIC_DRAW);
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), nullptr);
    glVertexAttribDivisor(3, 1); // 每个实例读一个 offset，而不是每个顶点读一个。
    glEnable(GL_DEPTH_TEST);
```

### 同主题跨 API 导航

以下是同主题参考；场景、功能完成度和版本基线可能不同，具体边界见对照章节。

| 路线 | 可运行参照 / 当前范围 |
| --- | --- |
| OpenGL | [12_instancing](README.md) |
| Vulkan 原生 | [vk04_vertex_buffer](../../../Vulkan/native/lessons/vk04_vertex_buffer/README.md) |
| Vulkan / nvpro_core2 | [nv04_vma_staging_texture](../../../Vulkan/nvpro/lessons/nv04_vma_staging_texture/README.md) |
| DirectX 12 | [dx04_vertex_buffer](../../../DirectX12/lessons/dx04_vertex_buffer/README.md) |

### 先预测，再验证

把 divisor 改为零后为什么立方体会扭曲？

参考推导：offset 变成每顶点读取，顶点各自得到不同偏移；这已经不是每实例放置整个立方体。

在一次实验中只改一个因素，记录预测、实际画面/日志和原因。检查已有 smoke 是否仍通过；图形调试器里再核对本次 draw 的实际输入/输出，不能仅凭进程正常退出下结论。
