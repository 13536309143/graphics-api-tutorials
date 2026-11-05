# 第 24 课：PBR 贴图工作流

## 本课独立工程与关键注释

[main.cpp](main.cpp) · [本地辅助实现](support/) · [本课着色器](shaders/) · [CMakeLists.txt](CMakeLists.txt)

本目录拥有完整课程源码、资源与依赖配方。关键步骤含中文注释，不链接其他课的课程实现。可将整个目录复制到其他位置，用以下命令单独构建和检查；第三方依赖首次配置需要联网或提供缓存。

```powershell
cmake -S . -B build -A x64
cmake --build build --config Debug --parallel 4
ctest --test-dir build -C Debug --output-on-failure
```

[课程首页](../../README.md) · [上一课](../23_pbr/README.md) · [下一课](../25_ibl/README.md)

## 目标与运行

```powershell
cmake --preset opengl
cmake --build --preset opengl-debug --target 24_pbr_textures --parallel 4
.\OpenGL\build\bin\Debug\24_pbr_textures.exe
```

预期画面：35 颗带纹理球，不同金属度/粗糙度因子下有空间变化的材质，空格关闭法线。

相机：W/A/S/D 移动，Q/E 升降，按住右键转向，Esc 退出。

## 代码阅读顺序

albedo.png / normal.png / metalrough.png / ao.png → pbr.frag useMaps 分支

从本课 [main.cpp](main.cpp) 进入；公共工具在 [common](../../common)，GLSL 在 [shaders](../../shaders)。先画出每一遍读什么、写什么，再逐个展开辅助类。

## 原理与实现

PBR 材质的各张图有不同语义：albedo 是没有烘焙直接光的基础颜色；normal 是切线空间法线；metallic 和 roughness 是标量；AO 描述小尺度环境遮蔽，不应无条件乘到直接光上。

本例遵循 glTF 的金属粗糙度通道：G 存粗糙度、B 存金属度，R 此处未参与计算；AO 独立读取 R。packed ORM 常见布局为 R=AO/G=roughness/B=metallic，但是否打包必须看素材约定，不能仅凭文件名猜。课程模型材质导入与此布局一致。

基础颜色贴图上传为 GL_SRGB8_ALPHA8，其他图为 GL_RGBA8；读取时只有颜色图自动解码。颜色 factor×贴图、粗糙度 factor×G、金属度 factor×B 允许同一组纹理复用于不同对象。法线图需要 TBN，且应该在片段内归一化。

本课的示例图是自制变化图，目的是看清每个通道对结果的影响，不是扫描得到的真实材质。导入外部材质包时首先确认 OpenGL/DirectX 法线绿色方向、粗糙度还是光滑度、色彩空间与通道打包规则。

## 修改练习

把 G 和 B 交换看错误；将 roughness 当成 1-glossiness 接入；用纯白 AO 对照。

修改后重新执行 `cmake --build --preset opengl-debug --target 24_pbr_textures --parallel 4`。GLSL、模型、图片会复制到程序旁的 shaders/assets 目录。源文件应修改教程根目录下的版本，运行目录的副本会被下一次构建覆盖。

## 自测与参考答案

AO 应直接让灯照到的表面也同样变黑吗？

本例只让 AO 调制环境项，直接光仍按几何与光源计算；具体分离策略取决于渲染模型。

## 检查与排错

运行 `24_pbr_textures.exe --smoke` 可执行确定时间的四帧渲染检查。重要技术开关还可用 `--smoke --variant --capture` 与默认抓图对比。共享工具会检查 GL 错误、非背景像素和图像动态范围，但这不能代替人工核对技术含义。参见 [进阶排错](../../docs/11_进阶排错与性能.md)。


## 参考截图

以下为课程自带素材、固定时间 1 秒时的实际渲染；你的相机与操作改变后画面会不同。

![24_pbr_textures](../../docs/previews/24_pbr_textures.png)

[查看按住空格时的对照图](../../docs/previews/24_pbr_textures_off.png)


## 深入学习：数据、接口与执行

albedo 做 sRGB 解码，normal/MR/AO 保持线性。glTF 常见 MR 的 G 为 roughness、B 为 metallic，不能把整张 RGB 当成一种标量或随意交换通道。

配套精讲：[三 API 对照 · 直接光 PBR 与 IBL](../../../docs/compare/12_lighting_pbr_ibl.md)。先读概念与参数选择，再返回本课实现核对，不必一次展开所有公共类。

### 本课的真实源码切片

来自 [OpenGL/lessons/24_pbr_textures/main.cpp](main.cpp#L7)，第 7—32 行；这是现有实现的局部摘录，末尾可能处于函数中间，不是可单独编译的完整程序。

```cpp
int main(int argc, char** argv) try {
    // 先创建上下文；它必须比 shader、纹理和网格活得更久。
    App app(argc, argv, "24_pbr_textures - OpenGL Course");
    // shader 路径指向本课自己的文件；属性、uniform 与下方 C++ 设置对应。
    Shader shader(app, "advanced.vert", "pbr.frag");
    Mesh ball(sphere());
    Camera camera;
    camera.pos = {0, 0, 14};
    camera.pitch = 0;
    ImageTexture albedo(asset(app, "textures/albedo.png"), true),
        normal(asset(app, "textures/normal.png")), mr(asset(app, "textures/metalrough.png")),
        ao(asset(app, "textures/ao.png"));
    glEnable(GL_DEPTH_TEST);
    // 每帧处理输入并绘制；GPU 对象在退出循环后按 C++ 作用域逆序释放。
    while (app.begin()) {
        camera.update(app);
        app.clear();
        shader.use();
        pbrDefaults(shader);
        cameraUniforms(shader, app, camera);
        albedo.bind(0);
        normal.bind(1);
        mr.bind(2);
        ao.bind(3);
        shader.set("useMaps", 1);
        shader.set("useNormal", app.key(GLFW_KEY_SPACE) != GLFW_PRESS);
```

### 同主题跨 API 导航

以下是同主题参考；场景、功能完成度和版本基线可能不同，具体边界见对照章节。

| 路线 | 可运行参照 / 当前范围 |
| --- | --- |
| OpenGL | [24_pbr_textures](README.md) |
| Vulkan 原生 | [vk09_pbr](../../../Vulkan/native/lessons/vk09_pbr/README.md) |
| Vulkan / nvpro_core2 | [nv06_pbr_viewport](../../../Vulkan/nvpro/lessons/nv06_pbr_viewport/README.md) |
| DirectX 12 | [dx09_pbr](../../../DirectX12/lessons/dx09_pbr/README.md) |

### 先预测，再验证

交换 MR 的 G/B 通道会发生什么？

参考推导：材质的粗糙度与金属度分布交换，高光形状和漫反射比例同时变化。用单通道调试图先确认输入。

在一次实验中只改一个因素，记录预测、实际画面/日志和原因。检查已有 smoke 是否仍通过；图形调试器里再核对本次 draw 的实际输入/输出，不能仅凭进程正常退出下结论。
