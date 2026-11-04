# dx36_integration · 模型纹理 mip MSAA 综合

[54 课目录](../../COURSE.md)

## 目的与实际执行

同样的 CPU 几何进入独立后端，texture view、MSAA resolve 和 CBV/PSO 使用期由 DX12 规则维护。CPU 生成正确 sRGB mip 与线性 GPU compute mip 是两条明确路径，不能混称同一种算法。

本程序有独立实验选择与验证路径，公共实现集中在 advanced_labs.cpp。设备/API 检查、数值 compute 与图形渲染采用不同证据；不是每个程序都会弹出窗口。

## 构建和运行

```powershell
cmake --preset dx12
cmake --build --preset dx12-debug --target dx36_integration --parallel 4
.\DirectX12\build\bin\Debug\dx36_integration.exe
```

## 深入学习：数据、接口与执行

- [第 54 课：综合场景、验收与工程化边界](../../chapters/54_capstone/README.md)

### 本课的真实源码切片

来自 [DirectX12/common/advanced_labs.cpp](../../common/advanced_labs.cpp#L502)，第 502—545 行；这是现有实现的局部摘录，末尾可能处于函数中间，不是可单独编译的完整程序。

```cpp
int graphicsLab(int argc, char** argv, const LabOptions& options) {
    int id = options.id, mode = id == 27 ? 3 : id == 29 || id == 53 ? 4 : 6;
    Geometry mesh;
    if (id == 21)
        mesh = staticAsset(options.directory / "assets/models/vase.obj");
    if (id == 22 || id == 36)
        mesh = staticAsset(options.directory / "assets/models/robot.gltf");
    if (id == 29)
        mesh = particleGeometry();
    if (id == 52 || id == 53) {
        mesh = geometry(4);
        for (auto& vertex : mesh.vertices) {
            vertex.normal =
                id == 53 ? glm::vec3(1 - vertex.uv.x, vertex.uv.y, 1) : glm::vec3(0, 0, 1);
            vertex.uv *= 4.f;
        }
        if (id == 52)
            for (uint32_t i = 0; i < mesh.vertices.size(); ++i)
                mesh.indices.push_back(i);
    }
    DxRenderOptions render;
    render.captureStem = "dx_lab_" + std::to_string(id);
    render.redOnly = id == 43;
    render.cull = id == 50 ? D3D12_CULL_MODE_BACK : D3D12_CULL_MODE_NONE;
    render.clampSampler = id == 52;
    if (!mesh.vertices.empty())
        render.mesh = &mesh;
    if (id == 20 || id == 36)
        render.samples = 4;
    render.setup = [=](DxScene& scene) {
        auto state = std::make_shared<RenderState>();
        scene.extension = state;
        if (id == 17 || id == 19 || id == 35 || id == 36 || id == 41 || id == 52) {
            auto pixels = imageFile(options.directory / "assets/textures/albedo.png");
            state->texture = std::make_unique<GpuTexture>(scene.c,
                                                          options,
                                                          std::move(pixels),
                                                          id == 19 || id == 36 || id == 41,
                                                          id == 35 || id == 36);
            state->texture->attach(scene, id == 41);
        }
        if (id == 20 || id == 36) {
            for (auto format : {DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_D32_FLOAT}) {
                D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS q{};
```

### 同主题跨 API 导航

| 路线 | 程序 |
| --- | --- |
| DirectX 12 | [dx36_integration](README.md) |
| 对照后端 | [vk36_integration](../../../Vulkan/native/lessons/vk36_integration/README.md) |

### 先预测，再验证

选择阴影、IBL 或骨骼动画之一，先写资源表、pass 依赖图、中间调试视图和验收条件，再实现。

参考推导：原生/DX12 当前完成静态模型、mip/MSAA、直接光 PBR 和计算扩展；完整 glTF 材质、skin/动画、IBL、SSAO 等仍需单独工程实现与验证。清楚标记边界比扩大功能名列表更有价值。

## 验证与边界

运行 `--smoke` 执行有限自动检查。数值实验读取实际 GPU 结果，图形实验检查非空图像并可查看读回。交互重建与性能还需额外证据；窗口 renderer 当前保留串行等待，双槽计算实验不会被描述为完整多帧呈现引擎。

## 实际画面

![dx36_integration](../../../docs/previews/dx36_integration.png)
