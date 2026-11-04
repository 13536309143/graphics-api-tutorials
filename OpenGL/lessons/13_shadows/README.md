# 第 13 课：阴影贴图

[课程首页](../../README.md) · [上一课](../12_instancing/README.md) · [下一课](../14_model_obj/README.md)

## 学完你应该能够

说明“从光源视角判断遮挡”，运行并修改本课示例。

## 运行与观察

在合集根目录执行（包含四条路线 preset 的目录）：

```powershell
cmake --preset opengl
cmake --build --preset opengl-debug --target 13_shadows --parallel 4
.\OpenGL\build\bin\Debug\13_shadows.exe
```

预期画面：地面上能看到两个立方体的阴影，旋转立方体的投影随之变化。

## 原理与代码阅读

阴影贴图由两遍渲染构成：第一遍从光源看场景，把最近表面的深度写到深度纹理；第二遍正常绘制，将当前片段变换到光源裁剪空间，比较它的深度是否大于纹理中记录的最近深度。

本例使用位置固定的白色主光，光照方向按点光源计算；阴影仅覆盖该光源朝原点的正交视锥，是用于说明深度比较的有限范围演示。它不提供点光源的全方向阴影；真实点光源阴影应使用 cubemap 六个方向，方向光则通常采用正交 shadow map 或级联方案。

lightSpace=lightProjection*lightView；片段投影后除以 w，再从 [-1,1] 映射到 [0,1]。深度纹理只接到 FBO 的 GL_DEPTH_ATTACHMENT，使用 glDrawBuffer(GL_NONE)/glReadBuffer(GL_NONE)。采样时使用独立单元 1，颜色纹理仍在单元 0。

同一表面的浮点误差会造成 shadow acne；bias 给比较留出余量，过大又会让阴影脱离物体（peter panning）。本例使用依赖 N·L 的 bias 和 3×3 PCF，让边缘稍柔和。纹理边界设白色深度 1，超出光源远平面的片段不加阴影；这些是有限视锥的处理，不是无限范围阴影。

观察地面上的旋转方块投影，按空格关闭阴影作对照。第一遍/第二遍必须用同一帧物体变换；视口也必须从 2048×2048 恢复成窗口大小。

首先阅读 [main.cpp](main.cpp)，再追踪其中调用的 [common/app.hpp](../../common/app.hpp) 和 [GLSL 文件](../../shaders)。不要一次通读所有公共工具：先弄清本课调用路径，再展开对应实现。

## 动手练习

修改 ShadowMap::size 看锯齿；把 bias 设 0 看 acne；关闭 PCF 比较边缘。

每次修改 C++ 或 GLSL 后，从根目录执行 `cmake --build --preset opengl-debug --target 13_shadows --parallel 4`。运行同一示例对比修改前后；尝试先预测画面，再修改代码。

## 自测

为什么增大 shadow map 分辨率也不总能解决阴影问题？

参考答案：深度精度、光源投影范围、bias 和采样方式同样影响质量，应先让光源视锥紧贴有效场景。

## 出错时优先检查

初始化顺序、当前程序、VAO/纹理绑定、uniform 值与坐标范围。Shader 语法错误会显示编译日志，OpenGL 状态错误会在帧尾输出错误编号。参见 [排错手册](../../docs/06_排错手册.md)。


## 深入学习：数据、接口与执行

深度遍用 lightVP 保存从光源看到的最近表面，光照遍再比较。偏移修正有限精度，不应靠极大的 bias 隐藏所有问题；先看深度图是否覆盖场景。

配套精讲：[三 API 对照 · 阴影、SSAO 与后处理](../../../docs/compare/13_multipass_effects.md)。先读概念与参数选择，再返回本课实现核对，不必一次展开所有公共类。

### 本课的真实源码切片

来自 [OpenGL/lessons/13_shadows/main.cpp](main.cpp#L5)，第 5—30 行；这是现有实现的局部摘录，末尾可能处于函数中间，不是可单独编译的完整程序。

```cpp
int main(int argc, char** argv) try {
    App app(argc, argv, "13_shadows - OpenGL Course");
    Shader depth(app, "depth.vert", "depth.frag"), lit(app, "lit.vert", "lit.frag");
    Mesh mesh(cube());
    Texture texture;
    Camera camera;
    ShadowMap shadow;
    camera.pos = glm::vec3(0, 3.5f, 7);
    camera.pitch = -23;
    glEnable(GL_DEPTH_TEST);
    auto drawScene = [&](const Shader& s) {
        auto floor = glm::translate(glm::mat4(1), glm::vec3(0, -1, 0));
        floor = glm::scale(floor, glm::vec3(10, .2f, 10));
        s.set("model", floor);
        mesh.draw();
        auto box = glm::rotate(glm::mat4(1), app.time() * .3f, glm::vec3(0, 1, 0));
        s.set("model", box);
        mesh.draw();
        s.set("model", glm::translate(glm::mat4(1), glm::vec3(2, 0, -2)));
        mesh.draw();
    };
    while (app.begin()) {
        camera.update(app);
        glm::vec3 light(-3, 6, 3);
        auto lightSpace = glm::ortho(-7.f, 7.f, -7.f, 7.f, 1.f, 20.f) *
                          glm::lookAt(light, glm::vec3(0), glm::vec3(0, 1, 0));
```

### 同主题跨 API 导航

以下是同主题参考；场景、功能完成度和版本基线可能不同，具体边界见对照章节。

| 路线 | 可运行参照 / 当前范围 |
| --- | --- |
| OpenGL | [13_shadows](README.md) |
| Vulkan 原生 | 迁移设计：vk10 已有离屏/blit；完整 Bloom/SSAO 仍需资源、采样管线及逐遍依赖。 |
| Vulkan / nvpro_core2 | 迁移设计：RenderTarget 与 ImGui 仅是接入点；新增多遍效果仍要自己描述写后读。 |
| DirectX 12 | 迁移设计：dx10 是 CopyResource，迁移后处理需新增可采样目标和全屏像素管线。 |

### 先预测，再验证

阴影整体错位时应先改 bias 吗？

参考推导：先核对 lightVP、坐标转换和采样范围。bias 主要改善局部自遮挡，不是修复错误变换的手段。

在一次实验中只改一个因素，记录预测、实际画面/日志和原因。检查已有 smoke 是否仍通过；图形调试器里再核对本次 draw 的实际输入/输出，不能仅凭进程正常退出下结论。
