# 第 09 课：Phong 光照

## 本课独立工程与关键注释

[main.cpp](main.cpp) · [本地辅助实现](support/) · [本课着色器](shaders/) · [CMakeLists.txt](CMakeLists.txt)

本目录拥有完整课程源码、资源与依赖配方。关键步骤含中文注释，不链接其他课的课程实现。可将整个目录复制到其他位置，用以下命令单独构建和检查；第三方依赖首次配置需要联网或提供缓存。

```powershell
cmake -S . -B build -A x64
cmake --build build --config Debug --parallel 4
ctest --test-dir build -C Debug --output-on-failure
```

[课程首页](../../README.md) · [上一课](../08_camera/README.md) · [下一课](../10_materials/README.md)

## 学完你应该能够

说明“Phong：从几何到明暗”，运行并修改本课示例。

## 运行与观察

在合集根目录执行（包含四条路线 preset 的目录）：

```powershell
cmake --preset opengl
cmake --build --preset opengl-debug --target 09_lighting --parallel 4
.\OpenGL\build\bin\Debug\09_lighting.exe
```

预期画面：三个不同高光锐度的立方体，光照随时间改变。

## 原理与代码阅读

本课使用环境光 ambient、漫反射 diffuse、镜面反射 specular。环境项提供简化的基础亮度；漫反射强度为 max(dot(N,L),0)，N/L 必须归一化；镜面项用 reflect(-L,N) 与视线 V 的夹角，指数 shininess 控制高光集中程度。

worldPos、normal、eye 和 light 必须处于同一坐标空间。此例全部在世界空间计算。模型经过非均匀缩放时，法线不能直接用 model 变换，而应使用 transpose(inverse(mat3(model)))；随后在片段阶段再次 normalize，因为插值会改变长度。

光源绕场景运动，三个立方体分别使用 shininess=8、32、128。指数越高，高光越小越锐利，不代表物体整体更亮。光从背面照来时漫反射为 0，同时关闭该面的镜面项，避免背面漏高光。

Phong 是便于入门的经验模型，环境项并非真实全局照明。本例默认直接输出线性计算结果到普通颜色缓冲，用于观察公式；线性工作流与 sRGB/Gamma 的正确处理见进阶路线。

首先阅读 [main.cpp](main.cpp)，再追踪其中调用的 [common/app.hpp](../../common/app.hpp) 和 [GLSL 文件](../../shaders)。不要一次通读所有公共工具：先弄清本课调用路径，再展开对应实现。

## 动手练习

改光源高度；关闭环境项；对模型做非均匀缩放对比法线变换。

每次修改 C++ 或 GLSL 后，从根目录执行 `cmake --build --preset opengl-debug --target 09_lighting --parallel 4`。运行同一示例对比修改前后；尝试先预测画面，再修改代码。

## 自测

相机移动时漫反射和镜面反射分别如何变化？

参考答案：固定物体与光源时漫反射不依赖视线；镜面项包含 V，因此随相机变化。

## 出错时优先检查

初始化顺序、当前程序、VAO/纹理绑定、uniform 值与坐标范围。Shader 语法错误会显示编译日志，OpenGL 状态错误会在帧尾输出错误编号。参见 [排错手册](../../docs/06_排错手册.md)。


## 深入学习：数据、接口与执行

先确认 N、L、V 都在世界空间并已归一化，再讨论高光指数。插值后的法线长度会变化，片元阶段还要 normalize。漫反射不依赖相机，镜面项依赖视线。

配套精讲：[三 API 对照 · 直接光 PBR 与 IBL](../../../docs/compare/12_lighting_pbr_ibl.md)。先读概念与参数选择，再返回本课实现核对，不必一次展开所有公共类。

### 本课的真实源码切片

来自 [OpenGL/lessons/09_lighting/main.cpp](main.cpp#L7)，第 7—32 行；这是现有实现的局部摘录，末尾可能处于函数中间，不是可单独编译的完整程序。

```cpp
int main(int argc, char** argv) try {
    // 先创建上下文；它必须比 shader、纹理和网格活得更久。
    App app(argc, argv, "09_lighting - OpenGL Course");
    // shader 路径指向本课自己的文件；属性、uniform 与下方 C++ 设置对应。
    Shader shader(app, "lit.vert", "lit.frag");
    // CPU 顶点数据上传后，由 VAO 保存属性布局；布局必须匹配顶点 shader。
    Mesh mesh(cube());
    Texture texture;
    Camera camera;
    glEnable(GL_DEPTH_TEST);
    // 每帧处理输入并绘制；GPU 对象在退出循环后按 C++ 作用域逆序释放。
    while (app.begin()) {
        camera.update(app);
        app.clear();
        shader.use();
        texture.bind();
        shader.set("tex", 0);
        shader.set("shadowMap", 1);
        shader.set("view", camera.view());
        shader.set("projection", camera.projection(app));
        shader.set("lightSpace", glm::mat4(1));
        shader.set("shadows", 0);
        shader.set("eye", camera.pos);
        shader.set("light", glm::vec3(2 * std::sin(app.time()), 3, 2 * std::cos(app.time())));
        shader.set("textured", 0);
        shader.set("multi", 0);
```

### 同主题跨 API 导航

以下是同主题参考；场景、功能完成度和版本基线可能不同，具体边界见对照章节。

| 路线 | 可运行参照 / 当前范围 |
| --- | --- |
| OpenGL | [09_lighting](README.md) |
| Vulkan 原生 | [vk09_pbr](../../../Vulkan/native/lessons/vk09_pbr/README.md) |
| Vulkan / nvpro_core2 | [nv06_pbr_viewport](../../../Vulkan/nvpro/lessons/nv06_pbr_viewport/README.md) |
| DirectX 12 | [dx09_pbr](../../../DirectX12/lessons/dx09_pbr/README.md) |

### 先预测，再验证

固定物体和光源，只移动相机，哪种项应变化？

参考推导：镜面高光随 V 变化，漫反射的 N·L 不因此改变；若整个明暗乱转，先检查坐标空间而不是 shininess。

在一次实验中只改一个因素，记录预测、实际画面/日志和原因。检查已有 smoke 是否仍通过；图形调试器里再核对本次 draw 的实际输入/输出，不能仅凭进程正常退出下结论。
