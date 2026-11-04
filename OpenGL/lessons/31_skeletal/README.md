# 第 31 课：骨骼动画与蒙皮

[课程首页](../../README.md) · [上一课](../30_geometry/README.md) · [下一课](../32_particles/README.md)

## 目标与运行

```powershell
cmake --preset opengl
cmake --build --preset opengl-debug --target 31_skeletal --parallel 4
.\OpenGL\build\bin\Debug\31_skeletal.exe
```

预期画面：一根带两根骨骼的软柱随时间弯曲；空格显示 bind pose。

相机：W/A/S/D 移动，Q/E 升降，按住右键转向，Esc 退出。

## 代码阅读顺序

bending.gltf/animations/skins → Model::update/visit → ImportedMesh 权重 → model.vert

从本课 [main.cpp](main.cpp) 进入；公共工具在 [common](../../common)，GLSL 在 [shaders](../../shaders)。先画出每一遍读什么、写什么，再逐个展开辅助类。

## 原理与实现

骨骼动画分为动画时间采样、层级变换、蒙皮矩阵与顶点混合。课程读取真实 glTF 的 skin、关节索引、权重、inverse bind matrices 和旋转关键帧，不是在 shader 中按顶点高度临时套一个波浪函数。

动画通道按节点名称匹配。位置和缩放线性插值，旋转用 quaternion slerp，按父子顺序计算 nodeGlobal。时间用 ticksPerSecond 转换并对 duration 取模。本简化播放器只播放第一条动画，并将通道按线性/slerp 处理；STEP/CUBICSPLINE、动画混合与根运动不在支持范围内。

mesh 局部的 bone matrix=inverse(meshGlobal)*boneGlobal*inverseBind。顶点按权重加权 bone matrix 后，外面再乘 rootTransform*meshGlobal。这样明确剥离网格节点变换，避免重复乘一遍模型变换。权重取最大的四个并归一化，无权重顶点保持原始姿态。

骨骼 ID 使用整数属性 glVertexAttribIPointer，对应 ivec4，不能像普通 float 属性一样解释。此课每网格上限 48 骨骼，在 shader 中使用 mat4 数组；更大骨架需要 UBO/纹理缓冲等方案并检查设备限制。线性混合蒙皮会出现糖纸扭转与体积损失，双四元数蒙皮是后续方向。

## 修改练习

修改 glTF 的旋转关键帧；延长动画时长；给模型加第三根骨骼与权重；在 bind pose 对比蒙皮矩阵。

修改后重新执行 `cmake --build --preset opengl-debug --target 31_skeletal --parallel 4`。GLSL、模型、图片会复制到程序旁的 shaders/assets 目录。源文件应修改教程根目录下的版本，运行目录的副本会被下一次构建覆盖。

## 自测与参考答案

为何要乘 inverseBind？

它把原始模型顶点从绑定姿态对应的网格空间转换到关节的参考空间，之后再应用当前关节姿态。

## 检查与排错

运行 `31_skeletal.exe --smoke` 可执行确定时间的四帧渲染检查。重要技术开关还可用 `--smoke --variant --capture` 与默认抓图对比。共享工具会检查 GL 错误、非背景像素和图像动态范围，但这不能代替人工核对技术含义。参见 [进阶排错](../../docs/11_进阶排错与性能.md)。


## 参考截图

以下为课程自带素材、固定时间 1 秒时的实际渲染；你的相机与操作改变后画面会不同。

![31_skeletal](../../docs/previews/31_skeletal.png)

[查看按住空格时的对照图](../../docs/previews/31_skeletal_off.png)


## 深入学习：数据、接口与执行

逐秒采样动画后累计节点 global，再用 inverseBind 形成网格局部骨骼矩阵。关节 ID 是整数，四权重归一化；本播放器只处理第一条动画的简化插值。

配套精讲：[三 API 对照 · 模型、节点与骨骼迁移](../../../docs/compare/11_models_animation.md)。先读概念与参数选择，再返回本课实现核对，不必一次展开所有公共类。

### 本课的真实源码切片

来自 [OpenGL/lessons/31_skeletal/main.cpp](main.cpp#L5)，第 5—30 行；这是现有实现的局部摘录，末尾可能处于函数中间，不是可单独编译的完整程序。

```cpp
int main(int argc, char** argv) try {
    App app(argc, argv, "31_skeletal - OpenGL Course");
    auto modelPath = asset(app, "models/bending.gltf");
    for (int k = 1; k + 1 < argc; ++k)
        if (std::string(argv[k]) == "--model")
            modelPath = std::filesystem::absolute(argv[++k]);
    Model model(modelPath);
    Shader shader(app, "model.vert", "pbr.frag");
    Camera camera;
    camera.pos = glm::vec3(0, 1.5f, 6);
    camera.pitch = -6;
    camera.yaw = -90;
    glEnable(GL_DEPTH_TEST);
    while (app.begin()) {
        camera.update(app);
        app.clear();
        shader.use();
        pbrDefaults(shader);
        cameraUniforms(shader, app, camera);
        // 空格按下时暂停在 bind pose，便于对照骨骼形变。
        model.update(app.key(GLFW_KEY_SPACE) == GLFW_PRESS ? 0.f : app.time());
        model.draw(shader);
        app.end();
    }
    return 0;
} catch (const std::exception& e) {
```

### 同主题跨 API 导航

以下是同主题参考；场景、功能完成度和版本基线可能不同，具体边界见对照章节。

| 路线 | 可运行参照 / 当前范围 |
| --- | --- |
| OpenGL | [31_skeletal](README.md) |
| Vulkan 原生 | 迁移设计：设计：CPU MeshData → VkBuffer；骨骼 UBO/SSBO；节点/材质解析可共享。 |
| Vulkan / nvpro_core2 | 迁移设计：框架有资产相关辅助，但本课程未接入完整 glTF/skin；需要单独实现并验收。 |
| DirectX 12 | 迁移设计：设计：扩展 VBV/input layout 加 joints/weights，骨骼经 CBV/SRV 提供。 |

### 先预测，再验证

静态正确但运动后网格炸开，应先查什么？

参考推导：先核对 joint index、权重、矩阵空间和 tick 单位，再查材质。对照 bind pose 能隔离动画采样与静态几何。

在一次实验中只改一个因素，记录预测、实际画面/日志和原因。检查已有 smoke 是否仍通过；图形调试器里再核对本次 draw 的实际输入/输出，不能仅凭进程正常退出下结论。
