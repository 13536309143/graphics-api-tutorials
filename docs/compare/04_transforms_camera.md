# 04 · 变换、相机、深度范围与坐标约定

[对照路线](../COMPARE.md) · [交互式对照](index.html#04_transforms_camera) · [上一章](03_geometry_memory.md) · [下一章](05_binding_constants.md)

换 API 时常见的“倒着、镜像、看不见”通常不是矩阵库坏了，而是把存储、乘法、坐标约定和 viewport 映射混成一件事。本章用同一个点走完数学链路。

## 从局部空间到屏幕

本仓库采用列向量记法：`clip = projection * view * model * local`。model 把模型局部点放到世界中；view 是相机世界变换的逆；projection 输出裁剪空间。裁剪之后还要除以 w 得到 NDC，最后经 viewport 映射为像素位置。CPU 不应该提前把所有位置除以 w 后再按普通三维位置传给标准管线。

矩阵“按列存储”只描述连续内存如何排列；它不自动决定左乘/右乘或左右手空间。GLM 默认矩阵内存和 HLSL `column_major` 在本项目中保持一致，HLSL 使用 `mul(M, v)`。移植时同时核对数学记法、CPU 内存和 shader 声明，不能只随意加一次 transpose。

| 项目中的约定 | OpenGL | Vulkan / nvpro | DX12 |
| --- | --- | --- | --- |
| 世界空间 | 右手 | 共享右手世界 | 共享右手世界 |
| NDC 深度 | -1..1 | 0..1 | 0..1 |
| 投影 Y 处理 | 原课程约定 | 正高度 viewport，投影 Y 翻转 | 不使用该次翻转 |
| 常量矩阵访问 | GLSL mat4 | GLSL mat4 | column_major float4x4 |

这里描述当前代码，不是断言 DX12 必须左手或 Vulkan 永远必须修改投影。Vulkan 也可采用其他合法 viewport/投影组合。关键是只做一次约定转换，并同步考虑 front-face、纹理方向和读回图像方向。

## 一个可手算的检查

先取局部点 `(1,0,0,1)`，model 只沿 X 平移 2，世界位置应为 `(3,0,0,1)`。如果先旋转再平移，与先平移再旋转一般不同。写出两个矩阵乘积并在 CPU 打印结果，比看到模型绕错轴后反复交换代码更容易定位。

OpenGL 相机课在右键按下时更新 yaw/pitch，把俯仰角限制在接近 ±90° 以内。移动速度乘 dt，避免高帧率下走得更快。Vulkan/DX12 除固定场景外，新增 23 课轨道相机，并按实际 aspect 更新投影；这不等于已移植 OpenGL 的完整自由相机交互。

## 法线为什么不等同于位置

法线描述表面的垂直方向。模型存在非均匀缩放时，用 model 的左上 3×3 直接变换法线会破坏垂直关系，应使用逆转置并在片元阶段归一化。Vulkan 场景顶点 shader 使用该形式；DX12 目前场景仅有旋转和均匀缩放，因此采用简化形式。把非均匀缩放练习迁过去前，先修正这一约定。

## 实验与答案

1. 把物体放在相机后方，再移回前方。先预测裁剪结果，确认“离原点很近”不等于“在相机前面”。
2. 将窗口从宽屏改为接近正方形。正确更新 aspect 后球体仍应接近球形；viewport 和投影 aspect 都要检查。
3. 非均匀缩放一个有法线高光的物体，比较直接 model 与逆转置两种法线。参考答案：差异来自法线方向而非顶点位置；只修正顶点矩阵不能修正光照。

抓帧时先看 VS 输出的 clip position，再看深度测试，最后看光照。这样可以把数学问题与材质问题分开。

<!-- generated-source-comparison -->

## 与本仓库实现并排核对

以下切片直接取自当前仓库，省略了前后文。行号用于定位，不能把片段当作完整可编译程序。

| 路线 | 可运行参照 / 范围 |
| --- | --- |
| OpenGL | [08_camera](../../OpenGL/lessons/08_camera/README.md) |
| Vulkan 原生 | [vk05_indexed_uniform](../../Vulkan/native/lessons/vk05_indexed_uniform/README.md) |
| Vulkan / nvpro_core2 | [nv06_pbr_viewport](../../Vulkan/nvpro/lessons/nv06_pbr_viewport/README.md) |
| DirectX 12 | [dx05_indexed_cbv](../../DirectX12/lessons/dx05_indexed_cbv/README.md) |

### OpenGL · 已实现

运动乘 dt；pitch 受限；view 与 projection 分别负责不同空间。

[OpenGL/lessons/08_camera/support/app.hpp](../../OpenGL/lessons/08_camera/support/app.hpp#L334)，第 334—383 行：

```cpp
struct Camera {
    glm::vec3 pos{0, 1.2f, 6}, front{0, 0, -1};
    float yaw = -90, pitch = -8, fov = 45;
    double oldX = 0, oldY = 0;
    bool first = true;
    void update(App& a) {
        // 按住右键才捕获鼠标，松开后可以正常操作桌面。
        bool capture = glfwGetMouseButton(a.window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
        glfwSetInputMode(
            a.window, GLFW_CURSOR, capture ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
        double x, y;
        glfwGetCursorPos(a.window, &x, &y);
        if (capture && !first) {
            yaw += float(x - oldX) * .1f;
            pitch = std::clamp(pitch + float(oldY - y) * .1f, -89.f, 89.f);
        }
        first = !capture;
        oldX = x;
        oldY = y;
        front =
            glm::normalize(glm::vec3(std::cos(glm::radians(yaw)) * std::cos(glm::radians(pitch)),
                                     std::sin(glm::radians(pitch)),
                                     std::sin(glm::radians(yaw)) * std::cos(glm::radians(pitch))));

        // 运动速度乘以帧间隔，避免移动速度随帧率改变。
        float speed = 3.f * a.dt;
        auto right = glm::normalize(glm::cross(front, glm::vec3(0, 1, 0)));
        if (glfwGetKey(a.window, GLFW_KEY_W) == GLFW_PRESS)
            pos += front * speed;
        if (glfwGetKey(a.window, GLFW_KEY_S) == GLFW_PRESS)
            pos -= front * speed;
        if (glfwGetKey(a.window, GLFW_KEY_A) == GLFW_PRESS)
            pos -= right * speed;
        if (glfwGetKey(a.window, GLFW_KEY_D) == GLFW_PRESS)
            pos += right * speed;
        if (glfwGetKey(a.window, GLFW_KEY_Q) == GLFW_PRESS)
            pos.y -= speed;
        if (glfwGetKey(a.window, GLFW_KEY_E) == GLFW_PRESS)
            pos.y += speed;
    }
    glm::mat4 view() const {
        return glm::lookAt(pos, pos + front, glm::vec3(0, 1, 0));
    }
    glm::mat4 projection(const App& a) const {
        return glm::perspective(glm::radians(fov), a.aspect(), .1f, 100.f);
    }
};
inline int failure(const std::exception& e) {
    std::cerr << "ERROR: " << e.what() << '\n';
    return 1;
```

### Vulkan 原生 · 已实现

共享右手空间，Vulkan 正 viewport 下额外翻转投影 Y。

[Vulkan/native/lessons/vk05_indexed_uniform/support/scene.hpp](../../Vulkan/native/lessons/vk05_indexed_uniform/support/scene.hpp#L86)，第 86—101 行：

```cpp
inline Frame frame(int mode, float aspect, bool vulkan) {
    Frame f;
    if (mode <= 4)
        return f;
    float distance = mode >= 9 ? 14.f : mode == 7 ? 10.f : 4.f;
    auto p = glm::perspectiveRH_ZO(glm::radians(45.f), aspect, .1f, 100.f);

    // 两条路线都使用 0..1 深度；Vulkan 正高度 viewport 额外翻转投影 Y。
    if (vulkan)
        p[1][1] *= -1;
    f.viewProjection = p * glm::lookAtRH(glm::vec3(0, mode >= 9 ? 0 : 1.5f, distance),
                                         glm::vec3(0),
                                         glm::vec3(0, 1, 0));
    return f;
}

```

### Vulkan / nvpro_core2 · 已实现

框架不改变 SceneVk 的数学约定；UI 调整的是材质参数。

[Vulkan/nvpro/lessons/nv06_pbr_viewport/support/scene_vk.hpp](../../Vulkan/nvpro/lessons/nv06_pbr_viewport/support/scene_vk.hpp#L414)，第 414—433 行：

```cpp
    void render(VkCommandBuffer cmd, VkExtent2D size, float time, float roughnessScale = 1) {
        auto f = overrideFrame.value_or(frame(mode, float(size.width) / size.height, true));
        if (frameProvider)
            f = frameProvider(float(size.width) / size.height, time, true);
        uniform.upload(&f, sizeof(f));
        VkViewport viewport{
            0, 0, float(size.width) / (halfViewport ? 2 : 1), float(size.height), 0, 1};
        VkRect2D scissor{{0, 0}, size};
        if (halfScissor)
            scissor.extent.width /= 2;
        vkCmdSetViewport(cmd, 0, 1, &viewport);
        vkCmdSetScissor(cmd, 0, 1, &scissor);
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
        vkCmdBindDescriptorSets(
            cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptor, 0, nullptr);
        if (mode >= 4) {
            VkBuffer vb = overrideVertices ? overrideVertices : vertices.buffer;
            VkDeviceSize offset = 0;
            vkCmdBindVertexBuffers(cmd, 0, 1, &vb, &offset);
            if (indices.buffer)
```

### DirectX 12 · 已实现

column_major / mul(M,v) 匹配 GLM；当前法线简化依赖旋转/均匀缩放。

[DirectX12/shaders/scene.hlsl](../../DirectX12/shaders/scene.hlsl#L25)，第 25—41 行：

```hlsl
Pixel VertexMain(Vertex input, uint instance : SV_InstanceID) {
    Pixel p;
    float3 local = input.pos;
    if ((int)settings.x == 7)
        local += float3((int)(instance % 5) - 2, 0, -(int)(instance / 5)) * 1.5;
    float4 world = mul(model, float4(local, 1));
    p.world = world.xyz;
    p.normal = mul((float3x3)model, input.normal);
    p.uv = input.uv;
    p.pos = mul(viewProjection, world);
    return p;
}

// SV_VertexID 直接生成顶点，适合先学习 PSO 和绘制提交。
Pixel TriangleMain(uint id : SV_VertexID) {
    float2 points[3] = {float2(-.7, -.6), float2(.7, -.6), float2(0, .6)};
    float3 colors[3] = {float3(1, 0, 0), float3(0, 1, 0), float3(0, 0, 1)};
```
