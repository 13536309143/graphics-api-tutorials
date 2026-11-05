# 第 05 课：纹理与采样

## 本课独立工程与关键注释

[main.cpp](main.cpp) · [本地辅助实现](support/) · [本课着色器](shaders/) · [CMakeLists.txt](CMakeLists.txt)

本目录拥有完整课程源码、资源与依赖配方。关键步骤含中文注释，不链接其他课的课程实现。可将整个目录复制到其他位置，用以下命令单独构建和检查；第三方依赖首次配置需要联网或提供缓存。

```powershell
cmake -S . -B build -A x64
cmake --build build --config Debug --parallel 4
ctest --test-dir build -C Debug --output-on-failure
```

[课程首页](../../README.md) · [上一课](../04_uniform/README.md) · [下一课](../06_transform/README.md)

## 学完你应该能够

说明“纹理就是可采样的数据”，运行并修改本课示例。

## 运行与观察

在合集根目录执行（包含四条路线 preset 的目录）：

```powershell
cmake --preset opengl
cmake --build --preset opengl-debug --target 05_texture --parallel 4
.\OpenGL\build\bin\Debug\05_texture.exe
```

预期画面：矩形显示橙蓝相间的棋盘纹理。

## 原理与代码阅读

纹理坐标 UV 通常覆盖 [0,1]。顶点着色器传出 UV，光栅化插值，片段着色器用 texture(tex,uv) 取样。sampler2D 中设置的是纹理单元序号 0，不是纹理对象 id，也不是 GL_TEXTURE0 常量。

Texture 类在 CPU 生成 64×64 的 RGB 棋盘图，glTexImage2D 上传到 GPU，因此此项目不依赖图片下载或图片解码库。图像的一行可能不满足默认 4 字节对齐；通用 RGB 上传可先设 GL_UNPACK_ALIGNMENT=1，上传后恢复。本例宽度恰好兼容 4 字节，也保留规范做法。

GL_TEXTURE_WRAP_S/T 控制越界 UV 的行为；GL_REPEAT 让图案重复。MIN_FILTER 使用 GL_LINEAR_MIPMAP_LINEAR，缩小时结合 mipmap，减轻闪烁。MAG_FILTER 只能使用 GL_NEAREST 或 GL_LINEAR，不能使用 mipmap 过滤选项。没有生成 mipmap 却要求 mipmap 的纹理可能不完整。

此课 texture.bind() 激活单元 0 并绑定纹理，shader.set("tex",0) 让 sampler 读取此单元。外部 PNG/JPEG 导入属于扩展练习：需要解码为像素，明确通道数、行方向和 sRGB 格式。后续 docs/08_进阶路线.md 给出接入步骤。

首先阅读 [main.cpp](main.cpp)，再追踪其中调用的 [common/app.hpp](../../common/app.hpp) 和 [GLSL 文件](../../shaders)。不要一次通读所有公共工具：先弄清本课调用路径，再展开对应实现。

## 动手练习

把 quad() 的 UV 从 1 改为 4，看重复；将过滤改成 GL_NEAREST 看像素边界。

每次修改 C++ 或 GLSL 后，从根目录执行 `cmake --build --preset opengl-debug --target 05_texture --parallel 4`。运行同一示例对比修改前后；尝试先预测画面，再修改代码。

## 自测

纹理对象 id=7，绑定单元 0 时 sampler 应设多少？

参考答案：设 0。对象编号与纹理单元编号属于两个不同概念。

## 出错时优先检查

初始化顺序、当前程序、VAO/纹理绑定、uniform 值与坐标范围。Shader 语法错误会显示编译日志，OpenGL 状态错误会在帧尾输出错误编号。参见 [排错手册](../../docs/06_排错手册.md)。


## 深入学习：数据、接口与执行

texture unit 是绑定点，sampler uniform 保存的是单元编号，不是 GLuint 纹理句柄。棋盘由 CPU 生成，可以先排除图片解码和文件路径问题，再检查 UV 与过滤。

配套精讲：[三 API 对照 · 纹理、Mip 与颜色空间](../../../docs/compare/06_textures_color.md)。先读概念与参数选择，再返回本课实现核对，不必一次展开所有公共类。

### 本课的真实源码切片

来自 [OpenGL/lessons/05_texture/main.cpp](main.cpp#L7)，第 7—29 行；这是现有实现的局部摘录，末尾可能处于函数中间，不是可单独编译的完整程序。

```cpp
int main(int argc, char** argv) try {
    // 先创建上下文；它必须比 shader、纹理和网格活得更久。
    App app(argc, argv, "05_texture - OpenGL Course");
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
| OpenGL | [05_texture](README.md) |
| Vulkan 原生 | [vk06_texture_descriptor](../../../Vulkan/native/lessons/vk06_texture_descriptor/README.md) |
| Vulkan / nvpro_core2 | [nv04_vma_staging_texture](../../../Vulkan/nvpro/lessons/nv04_vma_staging_texture/README.md) |
| DirectX 12 | [dx06_texture_srv](../../../DirectX12/lessons/dx06_texture_srv/README.md) |

### 先预测，再验证

把棋盘改成四象限颜色，发现上下相反时首先检查什么？

参考推导：检查 UV、CPU 行序和采样方向；改变光源不会修复纹理方向。先验证方向，再验证颜色空间。

在一次实验中只改一个因素，记录预测、实际画面/日志和原因。检查已有 smoke 是否仍通过；图形调试器里再核对本次 draw 的实际输入/输出，不能仅凭进程正常退出下结论。
