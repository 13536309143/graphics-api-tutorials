# 第 11 课：离屏渲染与后处理

[课程首页](../../README.md) · [上一课](../10_materials/README.md) · [下一课](../12_instancing/README.md)

## 学完你应该能够

说明“把渲染结果再次当作输入”，运行并修改本课示例。

## 运行与观察

在合集根目录执行（包含四条路线 preset 的目录）：

```powershell
cmake --preset opengl
cmake --build --preset opengl-debug --target 11_framebuffer --parallel 4
.\OpenGL\build\bin\Debug\11_framebuffer.exe
```

预期画面：灰度棋盘立方体；按住空格显示彩色原图。

## 原理与代码阅读

默认帧缓冲负责窗口显示，自定义 FBO 允许将图像渲染进纹理。本课 FBO 有 RGB8 颜色纹理和 DEPTH24_STENCIL8 renderbuffer。颜色之后要采样，选择纹理；深度只做测试，选择 renderbuffer。

第一遍绑定 target.fbo、启用深度测试、清除颜色和深度，然后画立方体。第二遍绑定 framebuffer 0，关闭深度测试，在全屏矩形上采样第一遍颜色。灰度公式 dot(rgb, vec3(0.2126,0.7152,0.0722)) 是线性 RGB 常用亮度权重；本课用于理解后处理，并未加入完整 sRGB 管线。

不能一边往某张纹理附件写入，一边在同一 pass 采样它，否则会形成未定义的反馈。若需要多次模糊，采用两张纹理交替读写。FBO 创建或缩放后使用 glCheckFramebufferStatus 检查完整性。

窗口缩放时颜色和深度附件一起重新分配，并使用新宽高；最小化到 0 像素时跳过分配。Framebuffer 类使用 RAII，退出时释放 GPU 对象。

首先阅读 [main.cpp](main.cpp)，再追踪其中调用的 [common/app.hpp](../../common/app.hpp) 和 [GLSL 文件](../../shaders)。不要一次通读所有公共工具：先弄清本课调用路径，再展开对应实现。

## 动手练习

反相输出 1-color；实现左右分屏；在 screen.frag 中加入 3×3 锐化卷积。

每次修改 C++ 或 GLSL 后，从根目录执行 `cmake --build --preset opengl-debug --target 11_framebuffer --parallel 4`。运行同一示例对比修改前后；尝试先预测画面，再修改代码。

## 自测

为什么第二遍要关闭深度测试？

参考答案：全屏后处理不需要场景深度，残留深度状态可能错误遮挡屏幕矩形。

## 出错时优先检查

初始化顺序、当前程序、VAO/纹理绑定、uniform 值与坐标范围。Shader 语法错误会显示编译日志，OpenGL 状态错误会在帧尾输出错误编号。参见 [排错手册](../../docs/06_排错手册.md)。


## 深入学习：数据、接口与执行

第一遍写 FBO 颜色纹理和深度，第二遍在默认目标画全屏四边形。第二遍通常不需要场景深度测试；切回默认目标时要恢复 viewport。

配套精讲：[三 API 对照 · 目标、深度与 MSAA](../../../docs/compare/07_targets_depth_msaa.md)。先读概念与参数选择，再返回本课实现核对，不必一次展开所有公共类。

### 本课的真实源码切片

来自 [OpenGL/lessons/11_framebuffer/main.cpp](main.cpp#L5)，第 5—30 行；这是现有实现的局部摘录，末尾可能处于函数中间，不是可单独编译的完整程序。

```cpp
int main(int argc, char** argv) try {
    App app(argc, argv, "11_framebuffer - OpenGL Course");
    Shader scene(app, "basic.vert", "texture.frag"), screen(app, "screen.vert", "screen.frag");
    Mesh box(cube()), plane(quad(), quadIndices());
    Texture texture;
    Camera camera;
    Framebuffer target;
    while (app.begin()) {
        if (app.width == 0 || app.height == 0) {
            app.end();
            continue;
        }
        camera.update(app);
        target.resize(app.width, app.height);
        // 第一遍：向离屏颜色纹理和深度附件画立方体。
        glBindFramebuffer(GL_FRAMEBUFFER, target.fbo);
        glEnable(GL_DEPTH_TEST);
        app.clear();
        scene.use();
        texture.bind();
        scene.set("tex", 0);
        auto model = glm::rotate(glm::mat4(1), app.time(), glm::vec3(.3f, 1, 0));
        scene.set("mvp", camera.projection(app) * camera.view() * model);
        box.draw();
        // 第二遍：默认帧缓冲上的全屏矩形。按住空格显示原图，松开为灰度。
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
```

### 同主题跨 API 导航

以下是同主题参考；场景、功能完成度和版本基线可能不同，具体边界见对照章节。

| 路线 | 可运行参照 / 当前范围 |
| --- | --- |
| OpenGL | [11_framebuffer](README.md) |
| Vulkan 原生 | [vk10_offscreen](../../../Vulkan/native/lessons/vk10_offscreen/README.md) |
| Vulkan / nvpro_core2 | [nv02_application](../../../Vulkan/nvpro/lessons/nv02_application/README.md) |
| DirectX 12 | [dx10_offscreen](../../../DirectX12/lessons/dx10_offscreen/README.md) |

### 先预测，再验证

如果后处理结果只出现在窗口一角，先查什么？

参考推导：先查 viewport 和全屏几何，再检查采样。新绑定目标不会自动把旧的小 viewport 改成窗口尺寸。

在一次实验中只改一个因素，记录预测、实际画面/日志和原因。检查已有 smoke 是否仍通过；图形调试器里再核对本次 draw 的实际输入/输出，不能仅凭进程正常退出下结论。
