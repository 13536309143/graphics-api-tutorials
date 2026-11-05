# 第 23 课：Cook-Torrance PBR

## 本课独立工程与关键注释

[main.cpp](main.cpp) · [本地辅助实现](support/) · [本课着色器](shaders/) · [CMakeLists.txt](CMakeLists.txt)

本目录拥有完整课程源码、资源与依赖配方。关键步骤含中文注释，不链接其他课的课程实现。可将整个目录复制到其他位置，用以下命令单独构建和检查；第三方依赖首次配置需要联网或提供缓存。

```powershell
cmake -S . -B build -A x64
cmake --build build --config Debug --parallel 4
ctest --test-dir build -C Debug --output-on-failure
```

[课程首页](../../README.md) · [上一课](../22_bloom/README.md) · [下一课](../24_pbr_textures/README.md)

## 目标与运行

```powershell
cmake --preset opengl
cmake --build --preset opengl-debug --target 23_pbr --parallel 4
.\OpenGL\build\bin\Debug\23_pbr.exe
```

预期画面：35 颗球：横轴粗糙度从低到高，纵轴金属度从低到高。

相机：W/A/S/D 移动，Q/E 升降，按住右键转向，Esc 退出。

## 代码阅读顺序

pbr.frag 的 directBRDF、distributionGGX、geometrySchlick、fresnel

从本课 [main.cpp](main.cpp) 进入；公共工具在 [common](../../common)，GLSL 在 [shaders](../../shaders)。先画出每一遍读什么、写什么，再逐个展开辅助类。

## 原理与实现

PBR 是以一致的材质与光照约定组织着色，不是某个 OpenGL 专属 API。本课采用金属度/粗糙度工作流的 Cook-Torrance 微表面模型：镜面 BRDF 为 D*G*F/(4*NdotV*NdotL)，再与入射辐射度和 NdotL 相乘。

D 使用 GGX 法线分布，粗糙度 r 先转换 alpha=r²，再使用 alpha² 进入分布公式；不要无意识重复平方。G 使用 Smith 思路的 Schlick-GGX 近似，直接光中的 k=(r+1)²/8。F 使用 Schlick 近似，介电材质 F0=0.04，金属的 F0 由基础颜色决定。

漫反射使用 Lambert albedo/π，比例 kD=(1-F)*(1-metallic)，避免镜面与漫反射同时消耗全部能量。金属度=1 时漫反射关闭。这是单次散射的教学模型，高粗糙度金属会损失部分多次散射能量，不等同于完整生产 PBR。

四个点光源按距离平方衰减，所有计算在线性空间。roughness 最小限制为 0.045，避免分布过尖引发数值问题；分母也设保护值。最后 Reinhard 映射与简化 Gamma 编码统一输出。本课小环境项只是背景补光，真实 IBL 在第 25 课实现。理论延伸参见 [Filament 的材质与光照设计](https://google.github.io/filament/Filament.md.html)。

## 修改练习

把金属度固定为 0 和 1；取消 kD 中 (1-metal) 看不守恒效果；比较 GGX 与旧 Phong 的粗糙度控制。

修改后重新执行 `cmake --build --preset opengl-debug --target 23_pbr --parallel 4`。GLSL、模型、图片会复制到程序旁的 shaders/assets 目录。源文件应修改教程根目录下的版本，运行目录的副本会被下一次构建覆盖。

## 自测与参考答案

金色金属的黄色主要属于漫反射还是镜面反射？

在该模型中金属漫反射为零，黄色主要体现在镜面反射 F0。

## 检查与排错

运行 `23_pbr.exe --smoke` 可执行确定时间的四帧渲染检查。重要技术开关还可用 `--smoke --variant --capture` 与默认抓图对比。共享工具会检查 GL 错误、非背景像素和图像动态范围，但这不能代替人工核对技术含义。参见 [进阶排错](../../docs/11_进阶排错与性能.md)。


## 参考截图

以下为课程自带素材、固定时间 1 秒时的实际渲染；你的相机与操作改变后画面会不同。

![23_pbr](../../docs/previews/23_pbr.png)


## 深入学习：数据、接口与执行

球阵列改变 metallic/roughness，其他条件保持可控。逐步检查 N、V、L、D、G、F，再组合镜面和漫反射；roughness 的平方约定应在不同 shader 中一致。

配套精讲：[三 API 对照 · 直接光 PBR 与 IBL](../../../docs/compare/12_lighting_pbr_ibl.md)。先读概念与参数选择，再返回本课实现核对，不必一次展开所有公共类。

### 本课的真实源码切片

来自 [OpenGL/lessons/23_pbr/main.cpp](main.cpp#L7)，第 7—32 行；这是现有实现的局部摘录，末尾可能处于函数中间，不是可单独编译的完整程序。

```cpp
int main(int argc, char** argv) try {
    // 先创建上下文；它必须比 shader、纹理和网格活得更久。
    App app(argc, argv, "23_pbr - OpenGL Course");
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
        shader.set("useMaps", 0);
        shader.set("useNormal", 0);
```

### 同主题跨 API 导航

以下是同主题参考；场景、功能完成度和版本基线可能不同，具体边界见对照章节。

| 路线 | 可运行参照 / 当前范围 |
| --- | --- |
| OpenGL | [23_pbr](README.md) |
| Vulkan 原生 | [vk09_pbr](../../../Vulkan/native/lessons/vk09_pbr/README.md) |
| Vulkan / nvpro_core2 | [nv06_pbr_viewport](../../../Vulkan/nvpro/lessons/nv06_pbr_viewport/README.md) |
| DirectX 12 | [dx09_pbr](../../../DirectX12/lessons/dx09_pbr/README.md) |

### 先预测，再验证

金属度提高一定更亮吗？

参考推导：不一定；漫反射减少、镜面颜色变化，实际亮度仍依赖方向与照明。先固定输入再比较。

在一次实验中只改一个因素，记录预测、实际画面/日志和原因。检查已有 smoke 是否仍通过；图形调试器里再核对本次 draw 的实际输入/输出，不能仅凭进程正常退出下结论。
