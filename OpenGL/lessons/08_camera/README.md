# 第 08 课：自由相机

[课程首页](../../README.md) · [上一课](../07_cube/README.md) · [下一课](../09_lighting/README.md)

## 学完你应该能够

说明“把输入变成相机运动”，运行并修改本课示例。

## 运行与观察

在合集根目录执行（包含四条路线 preset 的目录）：

```powershell
cmake --preset opengl
cmake --build --preset opengl-debug --target 08_camera --parallel 4
.\OpenGL\build\bin\Debug\08_camera.exe
```

预期画面：十个棋盘立方体，可自由穿行观察。

## 原理与代码阅读

相机的位置 pos、朝向 front 和世界 up 定义 view。用 yaw 和 pitch 计算方向，pitch 限制在 [-89,89]，避免 front 与 up 平行时叉乘退化。右方向 normalize(cross(front,up)) 由右手坐标系确定。

鼠标的像素位移乘灵敏度改变角度。第一次捕获鼠标只记录位置，不计算大幅跳变；松开右键恢复指针。键盘移动距离是 speed*dt，不应每帧移动固定距离。此课 dt 上限为 0.05 秒，防止调试暂停后突然飞远；这会在极低帧率时牺牲真实时间一致性。

W/S 前后，A/D 横移，Q/E 升降，按住右键转向。W/S 沿 front 移动，所以看向上方时会同时升高；如果想做贴地角色，应将 front 的 y 清零后归一化。此教学相机不包含碰撞，能穿过物体。

阅读 Camera::update/view/projection，再看第 08 课使用同一个 VP 绘制 10 个不同 model 的对象。相机并不移动 GPU 内部的某个实体；改变 view 矩阵即可改变观察方式。

首先阅读 [main.cpp](main.cpp)，再追踪其中调用的 [common/app.hpp](../../common/app.hpp) 和 [GLSL 文件](../../shaders)。不要一次通读所有公共工具：先弄清本课调用路径，再展开对应实现。

## 动手练习

加入 Shift 加速；改为地面移动；实现按 R 复位相机。

每次修改 C++ 或 GLSL 后，从根目录执行 `cmake --build --preset opengl-debug --target 08_camera --parallel 4`。运行同一示例对比修改前后；尝试先预测画面，再修改代码。

## 自测

为什么鼠标旋转不直接乘 dt？

参考答案：本例输入是每帧累计像素位移，已经随采样间隔变化；再乘 dt 会引入帧率依赖。

## 出错时优先检查

初始化顺序、当前程序、VAO/纹理绑定、uniform 值与坐标范围。Shader 语法错误会显示编译日志，OpenGL 状态错误会在帧尾输出错误编号。参见 [排错手册](../../docs/06_排错手册.md)。


## 深入学习：数据、接口与执行

相机移动实际改变 view，即相机世界变换的逆。鼠标更新 yaw/pitch，键盘沿 front/right 移动，速度乘 dt。右键捕获鼠标与渲染状态无关，是输入交互策略。

配套精讲：[三 API 对照 · 矩阵、相机与坐标](../../../docs/compare/04_transforms_camera.md)。先读概念与参数选择，再返回本课实现核对，不必一次展开所有公共类。

### 本课的真实源码切片

来自 [OpenGL/lessons/08_camera/main.cpp](main.cpp#L5)，第 5—30 行；这是现有实现的局部摘录，末尾可能处于函数中间，不是可单独编译的完整程序。

```cpp
int main(int argc, char** argv) try {
    App app(argc, argv, "08_camera - OpenGL Course");
    Shader shader(app, "basic.vert", "texture.frag");
    Mesh mesh(cube());
    Texture texture;
    Camera camera;
    glEnable(GL_DEPTH_TEST);
    while (app.begin()) {
        app.clear();
        shader.use();
        texture.bind();
        shader.set("tex", 0);
        camera.update(app);
        auto vp = camera.projection(app) * camera.view();
        for (int n = 0; n < 10; ++n) {
            auto model =
                glm::translate(glm::mat4(1), glm::vec3((n % 5 - 2) * 1.6f, 0, -(n / 5) * 2.f));
            model = glm::rotate(model, app.time() * .3f + n, glm::vec3(1, .5f, .2f));
            shader.set("mvp", vp * model);
            mesh.draw();
        }
        app.end();
    }
    return 0;
} catch (const std::exception& e) {
    return failure(e);
```

### 同主题跨 API 导航

以下是同主题参考；场景、功能完成度和版本基线可能不同，具体边界见对照章节。

| 路线 | 可运行参照 / 当前范围 |
| --- | --- |
| OpenGL | [08_camera](README.md) |
| Vulkan 原生 | [vk05_indexed_uniform](../../../Vulkan/native/lessons/vk05_indexed_uniform/README.md) |
| Vulkan / nvpro_core2 | [nv06_pbr_viewport](../../../Vulkan/nvpro/lessons/nv06_pbr_viewport/README.md) |
| DirectX 12 | [dx05_indexed_cbv](../../../DirectX12/lessons/dx05_indexed_cbv/README.md) |

### 先预测，再验证

把 dt 移除后，以不同帧率运行会发生什么？

参考推导：每帧固定步长会使单位时间移动速度随帧率改变。应分清每帧位移与每秒速度。

在一次实验中只改一个因素，记录预测、实际画面/日志和原因。检查已有 smoke 是否仍通过；图形调试器里再核对本次 draw 的实际输入/输出，不能仅凭进程正常退出下结论。
