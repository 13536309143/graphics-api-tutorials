# 第 10 课：材质与多光源

[课程首页](../../README.md) · [上一课](../09_lighting/README.md) · [下一课](../11_framebuffer/README.md)

## 学完你应该能够

说明“纹理材质与多光源叠加”，运行并修改本课示例。

## 运行与观察

在合集根目录执行（包含四条路线 preset 的目录）：

```powershell
cmake --preset opengl
cmake --build --preset opengl-debug --target 10_materials --parallel 4
.\OpenGL\build\bin\Debug\10_materials.exe
```

预期画面：棋盘材质受移动白光和固定蓝光共同照亮。

## 原理与代码阅读

材质描述表面对光的响应，不能只把它理解为一张图片。这里棋盘纹理调制基础颜色，shininess 控制高光指数，镜面强度暂时固定为 0.45。真正的材质还可以有独立漫反射贴图、镜面贴图与法线贴图。

此课在第 09 课白色光源之外，加入固定蓝色点光源，按光源逐项累加贡献。点光衰减为 1/(1+0.09*d+0.032*d*d)，参数用于可见的教学效果；它不是严格的物理逆平方模型。白色移动主光仍未应用距离衰减，便于与上一课对照。

颜色值可能在多光源叠加后超过 1，普通归一化颜色缓冲会截断，这也是 HDR 与 tone mapping 要解决的问题。不要通过把所有光源随意平均来代替能量叠加。

对比 lit.frag 中 phong 函数与 multi 分支。大量光源应使用结构体数组、UBO，甚至延迟渲染或 tiled/clustered lighting；本课先保持两盏灯的计算过程清晰。

首先阅读 [main.cpp](main.cpp)，再追踪其中调用的 [common/app.hpp](../../common/app.hpp) 和 [GLSL 文件](../../shaders)。不要一次通读所有公共工具：先弄清本课调用路径，再展开对应实现。

## 动手练习

加入第二个彩色点光；让三个物体使用不同镜面强度；为光源绘制小立方体作为标记。

每次修改 C++ 或 GLSL 后，从根目录执行 `cmake --build --preset opengl-debug --target 10_materials --parallel 4`。运行同一示例对比修改前后；尝试先预测画面，再修改代码。

## 自测

为什么更多光源可能造成大片纯白？

参考答案：光照求和超出 [0,1] 被颜色缓冲截断。可降低强度，进一步学习浮点 HDR 缓冲与色调映射。

## 出错时优先检查

初始化顺序、当前程序、VAO/纹理绑定、uniform 值与坐标范围。Shader 语法错误会显示编译日志，OpenGL 状态错误会在帧尾输出错误编号。参见 [排错手册](../../docs/06_排错手册.md)。


## 深入学习：数据、接口与执行

材质参数改变表面响应，光源参数改变入射照明。对比材质时尽量保持位置、法线、相机与光源一致；否则高光差异可能来自角度而非材质。

配套精讲：[三 API 对照 · 直接光 PBR 与 IBL](../../../docs/compare/12_lighting_pbr_ibl.md)。先读概念与参数选择，再返回本课实现核对，不必一次展开所有公共类。

### 本课的真实源码切片

来自 [OpenGL/lessons/10_materials/main.cpp](main.cpp#L5)，第 5—30 行；这是现有实现的局部摘录，末尾可能处于函数中间，不是可单独编译的完整程序。

```cpp
int main(int argc, char** argv) try {
    App app(argc, argv, "10_materials - OpenGL Course");
    Shader shader(app, "lit.vert", "lit.frag");
    Mesh mesh(cube());
    Texture texture;
    Camera camera;
    glEnable(GL_DEPTH_TEST);
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
        shader.set("textured", 1);
        shader.set("multi", 1);
        for (int n = 0; n < 3; ++n) {
            auto model = glm::translate(glm::mat4(1), glm::vec3((n - 1) * 1.7f, 0, 0));
            model = glm::rotate(model, app.time() * .2f, glm::vec3(.2f, 1, 0));
            shader.set("model", model);
```

### 同主题跨 API 导航

以下是同主题参考；场景、功能完成度和版本基线可能不同，具体边界见对照章节。

| 路线 | 可运行参照 / 当前范围 |
| --- | --- |
| OpenGL | [10_materials](README.md) |
| Vulkan 原生 | [vk09_pbr](../../../Vulkan/native/lessons/vk09_pbr/README.md) |
| Vulkan / nvpro_core2 | [nv06_pbr_viewport](../../../Vulkan/nvpro/lessons/nv06_pbr_viewport/README.md) |
| DirectX 12 | [dx09_pbr](../../../DirectX12/lessons/dx09_pbr/README.md) |

### 先预测，再验证

提高 shininess 为什么不是简单让物体更亮？

参考推导：指数主要收窄高光分布。整体能量与其他系数仍有关，不能把高光面积与整体亮度当成同一个量。

在一次实验中只改一个因素，记录预测、实际画面/日志和原因。检查已有 smoke 是否仍通过；图形调试器里再核对本次 draw 的实际输入/输出，不能仅凭进程正常退出下结论。
