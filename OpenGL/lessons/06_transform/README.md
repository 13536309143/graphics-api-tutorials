# 第 06 课：二维变换

## 本课独立工程与关键注释

[main.cpp](main.cpp) · [本地辅助实现](support/) · [本课着色器](shaders/) · [CMakeLists.txt](CMakeLists.txt)

本目录拥有完整课程源码、资源与依赖配方。关键步骤含中文注释，不链接其他课的课程实现。可将整个目录复制到其他位置，用以下命令单独构建和检查；第三方依赖首次配置需要联网或提供缓存。

```powershell
cmake -S . -B build -A x64
cmake --build build --config Debug --parallel 4
ctest --test-dir build -C Debug --output-on-failure
```

[课程首页](../../README.md) · [上一课](../05_texture/README.md) · [下一课](../07_cube/README.md)

## 学完你应该能够

说明“矩阵顺序与局部坐标”，运行并修改本课示例。

## 运行与观察

在合集根目录执行（包含四条路线 preset 的目录）：

```powershell
cmake --preset opengl
cmake --build --preset opengl-debug --target 06_transform --parallel 4
.\OpenGL\build\bin\Debug\06_transform.exe
```

预期画面：偏右的棋盘矩形持续旋转。

## 原理与代码阅读

平移、旋转、缩放可以合并为 4×4 矩阵。齐次坐标 vec4(position,1) 让平移也能通过乘法表达。方向向量通常使用 w=0，避免被平移影响。

GLM 与 GLSL 都使用列向量约定。本课代码依次 translate、rotate、scale 得到 T*R*S；实际作用到向量时从右往左执行，即先缩放，再旋转，最后平移。矩阵乘法不可交换，R*T 会让平移后的位置绕原点旋转。

本课先把矩形缩小到 0.6 倍，绕自身原点旋转，再向右平移 0.25。glm::rotate 的角度单位是弧度；如果用角度要先 glm::radians。glUniformMatrix4fv 用 GL_FALSE，不需要为了列主序再转置一次。

当前还没有透视相机，mvp 仅承担二维模型变换，所以宽窗口中图形比例可能被视口拉伸。第 07 课会用带 aspect 的透视投影解决三维场景比例。

首先阅读 [main.cpp](main.cpp)，再追踪其中调用的 [common/app.hpp](../../common/app.hpp) 和 [GLSL 文件](../../shaders)。不要一次通读所有公共工具：先弄清本课调用路径，再展开对应实现。

## 动手练习

交换 translate 和 rotate 的调用顺序；解释自转与绕转的变化。

每次修改 C++ 或 GLSL 后，从根目录执行 `cmake --build --preset opengl-debug --target 06_transform --parallel 4`。运行同一示例对比修改前后；尝试先预测画面，再修改代码。

## 自测

为什么每帧从单位矩阵重新开始？

参考答案：直接从时间计算姿态可避免重复累乘带来的误差，并使动画不依赖帧率。

## 出错时优先检查

初始化顺序、当前程序、VAO/纹理绑定、uniform 值与坐标范围。Shader 语法错误会显示编译日志，OpenGL 状态错误会在帧尾输出错误编号。参见 [排错手册](../../docs/06_排错手册.md)。


## 深入学习：数据、接口与执行

变换按列向量从右向左作用。围绕自身旋转与围绕世界原点公转，往往只差平移/旋转矩阵的乘法顺序。用一个点手算后再观察动画，更容易理解。

配套精讲：[三 API 对照 · 矩阵、相机与坐标](../../../docs/compare/04_transforms_camera.md)。先读概念与参数选择，再返回本课实现核对，不必一次展开所有公共类。

### 本课的真实源码切片

来自 [OpenGL/lessons/06_transform/main.cpp](main.cpp#L7)，第 7—32 行；这是现有实现的局部摘录，末尾可能处于函数中间，不是可单独编译的完整程序。

```cpp
int main(int argc, char** argv) try {
    // 先创建上下文；它必须比 shader、纹理和网格活得更久。
    App app(argc, argv, "06_transform - OpenGL Course");
    // shader 路径指向本课自己的文件；属性、uniform 与下方 C++ 设置对应。
    Shader shader(app, "basic.vert", "texture.frag");
    // CPU 顶点数据上传后，由 VAO 保存属性布局；布局必须匹配顶点 shader。
    Mesh mesh(quad(), quadIndices());
    Texture texture;
    // 每帧处理输入并绘制；GPU 对象在退出循环后按 C++ 作用域逆序释放。
    while (app.begin()) {
        app.clear();
        shader.use();
        glm::mat4 mvp(1.f);
        mvp = glm::translate(mvp, glm::vec3(.25f, 0, 0));
        mvp = glm::rotate(mvp, app.time(), glm::vec3(0, 0, 1));
        mvp = glm::scale(mvp, glm::vec3(.6f));
        shader.set("mvp", mvp);
        texture.bind();
        shader.set("tex", 0);
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
| OpenGL | [06_transform](README.md) |
| Vulkan 原生 | [vk05_indexed_uniform](../../../Vulkan/native/lessons/vk05_indexed_uniform/README.md) |
| Vulkan / nvpro_core2 | [nv06_pbr_viewport](../../../Vulkan/nvpro/lessons/nv06_pbr_viewport/README.md) |
| DirectX 12 | [dx05_indexed_cbv](../../../DirectX12/lessons/dx05_indexed_cbv/README.md) |

### 先预测，再验证

对局部点 (1,0,0) 先平移再旋转与先旋转再平移会相同吗？

参考推导：一般不同。写出两个乘积并打印变换结果，不要把矩阵在内存按列存储误认为乘法可交换。

在一次实验中只改一个因素，记录预测、实际画面/日志和原因。检查已有 smoke 是否仍通过；图形调试器里再核对本次 draw 的实际输入/输出，不能仅凭进程正常退出下结论。
