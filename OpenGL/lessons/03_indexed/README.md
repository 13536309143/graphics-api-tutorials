# 第 03 课：索引绘制与线框

## 本课独立工程与关键注释

[main.cpp](main.cpp) · [本地辅助实现](support/) · [本课着色器](shaders/) · [CMakeLists.txt](CMakeLists.txt)

本目录拥有完整课程源码、资源与依赖配方。关键步骤含中文注释，不链接其他课的课程实现。可将整个目录复制到其他位置，用以下命令单独构建和检查；第三方依赖首次配置需要联网或提供缓存。

```powershell
cmake -S . -B build -A x64
cmake --build build --config Debug --parallel 4
ctest --test-dir build -C Debug --output-on-failure
```

[课程首页](../../README.md) · [上一课](../02_triangle/README.md) · [下一课](../04_uniform/README.md)

## 学完你应该能够

说明“复用顶点与 EBO”，运行并修改本课示例。

## 运行与观察

在合集根目录执行（包含四条路线 preset 的目录）：

```powershell
cmake --preset opengl
cmake --build --preset opengl-debug --target 03_indexed --parallel 4
.\OpenGL\build\bin\Debug\03_indexed.exe
```

预期画面：渐变矩形；按住空格显示两个三角形的线框。

## 原理与代码阅读

矩形可拆成两个三角形。直接绘制需要 6 个顶点，但实际上只有 4 个角。EBO 存索引 0,1,2,2,3,0，使两个三角形复用顶点。此课用 glDrawElements，count=6 是索引数量。

GL_ELEMENT_ARRAY_BUFFER 的绑定属于当前 VAO 状态。创建 EBO 时必须绑定对应 VAO；在 VAO 仍然绑定时把 EBO 解绑，会把 VAO 的 EBO 关联清掉。glDrawElements 的最后一个参数是 EBO 内字节偏移，此处 nullptr 表示偏移 0。

默认按逆时针顶点顺序定义正面，本课未启用面剔除。按住空格把 glPolygonMode 切为 GL_LINE，可以看到矩形的对角线。索引必须匹配 GL_UNSIGNED_INT 类型且不能超出 VBO 顶点范围。

阅读 quad/quadIndices 与 Mesh 中 indexed 分支，再比较第 02 课。VAO 与 VBO 不是同一个对象；索引复用的也是完整顶点属性组合，而不只是位置。

首先阅读 [main.cpp](main.cpp)，再追踪其中调用的 [common/app.hpp](../../common/app.hpp) 和 [GLSL 文件](../../shaders)。不要一次通读所有公共工具：先弄清本课调用路径，再展开对应实现。

## 动手练习

改成菱形；有意删除最后三个索引，观察只剩半个矩形。

每次修改 C++ 或 GLSL 后，从根目录执行 `cmake --build --preset opengl-debug --target 03_indexed --parallel 4`。运行同一示例对比修改前后；尝试先预测画面，再修改代码。

## 自测

索引数据用 unsigned short 时要同时修改什么？

参考答案：EBO 上传元素大小与 glDrawElements 的 type 都要改成相应大小及 GL_UNSIGNED_SHORT。

## 出错时优先检查

初始化顺序、当前程序、VAO/纹理绑定、uniform 值与坐标范围。Shader 语法错误会显示编译日志，OpenGL 状态错误会在帧尾输出错误编号。参见 [排错手册](../../docs/06_排错手册.md)。


## 深入学习：数据、接口与执行

四边形由四套顶点属性与六个 uint 索引构成。DrawElements 的 count 是索引数，不是顶点数；EBO 绑定随 VAO 保存。先手画 0、1、2 和 2、3、0 两个三角形，再看屏幕。

配套精讲：[三 API 对照 · 顶点、索引与内存](../../../docs/compare/03_geometry_memory.md)。先读概念与参数选择，再返回本课实现核对，不必一次展开所有公共类。

### 本课的真实源码切片

来自 [OpenGL/lessons/03_indexed/main.cpp](main.cpp#L7)，第 7—30 行；这是现有实现的局部摘录，末尾可能处于函数中间，不是可单独编译的完整程序。

```cpp
int main(int argc, char** argv) try {
    // 先创建上下文；它必须比 shader、纹理和网格活得更久。
    App app(argc, argv, "03_indexed - OpenGL Course");
    // shader 路径指向本课自己的文件；属性、uniform 与下方 C++ 设置对应。
    Shader shader(app, "basic.vert", "color.frag");
    // CPU 顶点数据上传后，由 VAO 保存属性布局；布局必须匹配顶点 shader。
    Mesh mesh(quad(), quadIndices());
    // 每帧处理输入并绘制；GPU 对象在退出循环后按 C++ 作用域逆序释放。
    while (app.begin()) {
        app.clear();
        shader.use();
        glm::mat4 mvp(1.f);
        // 按住空格看线框：一个矩形由两个三角形组成。
        glPolygonMode(GL_FRONT_AND_BACK,
                      glfwGetKey(app.window, GLFW_KEY_SPACE) == GLFW_PRESS ? GL_LINE : GL_FILL);
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
| OpenGL | [03_indexed](README.md) |
| Vulkan 原生 | [vk04_vertex_buffer](../../../Vulkan/native/lessons/vk04_vertex_buffer/README.md) |
| Vulkan / nvpro_core2 | [nv04_vma_staging_texture](../../../Vulkan/nvpro/lessons/nv04_vma_staging_texture/README.md) |
| DirectX 12 | [dx04_vertex_buffer](../../../DirectX12/lessons/dx04_vertex_buffer/README.md) |

### 先预测，再验证

把 count 从六改为三应看到什么？

参考推导：只剩一个三角形。若变成随机刺状几何，应检查类型、偏移和缓冲内容，而不是先改投影。

在一次实验中只改一个因素，记录预测、实际画面/日志和原因。检查已有 smoke 是否仍通过；图形调试器里再核对本次 draw 的实际输入/输出，不能仅凭进程正常退出下结论。
