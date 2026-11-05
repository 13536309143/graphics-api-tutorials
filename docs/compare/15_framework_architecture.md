# 15 · nvpro_core2 与多后端架构

[对照路线](../COMPARE.md) · [交互式对照](index.html#15_framework_architecture) · [上一章](14_debug_performance.md)

框架能够减少重复创建和窗口管理，但学习时还要知道被隐藏的责任。等独立后端可以解释清楚之后，再抽象 Renderer 接口，比一开始就设计“万能 API”更容易保留正确性。

## nvpro 路线到底封装了什么

| 课程对象 | 帮助解决的问题 | 应用仍负责的内容 |
| --- | --- | --- |
| nvvk::Context | instance/device/queue 初始化 | 声明所需扩展与特性，判断失败原因 |
| ResourceAllocator / VMA | 分配、绑定、资源辅助管理 | usage、访问依赖、释放时机 |
| StagingUploader | 收集与录制上传工作 | 提交完成后回收临时数据 |
| nvapp::Application | 窗口、帧循环、UI 生命周期 | 自定义 pass 及其资源契约 |
| RenderTarget / ImTexture | 离屏附件及 UI 显示连接 | 格式、尺寸、布局/依赖及显示含义 |

本项目 `NvLesson` 实现 IAppElement：onAttach 创建长期资源，onResize 更新尺寸相关目标，onRender 录制场景，onDetach 释放依赖。onLastHeadlessFrame 读回用于检查。按这些回调阅读，比从 framework 所有文件开始通读更有效。

SceneVk 在原生路线和框架路线共享，这有意隔离“初始化与窗口设施”和“实际图形管线”。但它也意味着 nvpro 课程并没有重新用每个 nvvk helper 实现一份全部渲染器；教程应把复用处说清楚，不将共享代码包装成额外实现数量。

## 何时开始做统一接口

先要求每个后端独立画出同一固定场景，并能读回、抓帧、解释资源状态。随后把可共享的数据层抽出：Scene、MeshData、Material、Camera、DrawItem。让后端根据自身约束创建 GpuMesh/GpuTexture 和帧资源。

不要把 Vulkan stage/access/layout 原样塞进所有后端的公共接口，再让 OpenGL 忽略大部分参数；也不要把 OpenGL 的全局绑定模型套到 Vulkan/DX12，迫使后者在每次小调用中猜测后续用途。公共层更适合表达 pass 的读写意图，后端据此生成合法命令。

## 一个可逐步实现的边界

```text
场景/资产层：文件解析、节点、相机、材质、动画
渲染组织层：可见对象、pass、读写意图、绘制批次
后端层：上传、view/descriptor、pipeline、录制、同步、呈现
平台层：窗口、输入、文件、诊断
```

这只是设计图，当前仓库保留可独立学习的后端，没有声称已完成统一引擎。单个 Renderer::drawScene 方法可以是起点，但必须明确资源句柄的拥有者、提交后的生命周期和窗口重建通知。

## 迁移功能的正确顺序

当前原生/DX12 已加入静态模型、图片/mip、轨道相机、MSAA 和双槽计算；后续补完整材质/自由相机、阴影、HDR/IBL、动画及多帧呈现。每次保留上一个可以运行的状态，增加对应的中间调试视图。先做功能对齐，再做性能对齐，最后才尝试共享更多实现。

设备丢失恢复、bindless、ray tracing、多队列、多线程录制和完整渲染图可作为后续专题，但都不应因为 API 理论上支持就标记为本课程已经完成。功能矩阵把“已实现、仅设计、版本外”分开记录。

## 综合自测

把 nvpro 移除后哪些代码应该不变？参考答案：几何/数学、BRDF 和部分 SceneVk 可保持，Context、Application、RenderTarget/ImGui 接入及分配上传部分需要替代。反过来，给原生 Vulkan 接入 nvpro 并不需要把所有 shader 改成另一种语言。

最终练习：选择一项当前仅 OpenGL 完成的效果，写资源表、pass 依赖、三种 API 的绑定方案、调试视图和验收条件，再开始写代码。迁移设计的可解释性是本对照路线的毕业标准。

<!-- generated-source-comparison -->

## 与本仓库实现并排核对

以下切片直接取自当前仓库，省略了前后文。行号用于定位，不能把片段当作完整可编译程序。

| 路线 | 可运行参照 / 范围 |
| --- | --- |
| OpenGL | [34_gallery](../../OpenGL/lessons/34_gallery/README.md) |
| Vulkan 原生 | [vk09_pbr](../../Vulkan/native/lessons/vk09_pbr/README.md) |
| Vulkan / nvpro_core2 | [nv06_pbr_viewport](../../Vulkan/nvpro/lessons/nv06_pbr_viewport/README.md) |
| DirectX 12 | [dx09_pbr](../../DirectX12/lessons/dx09_pbr/README.md) |

### OpenGL · 已实现

先观察组合效果需要哪些对象，再分离平台、资产与渲染责任。

[OpenGL/lessons/34_gallery/main.cpp](../../OpenGL/lessons/34_gallery/main.cpp#L8)，第 8—35 行：

```cpp
int main(int argc, char** argv) try {
    // 先创建上下文；它必须比 shader、纹理和网格活得更久。
    App app(argc, argv, "34_gallery - OpenGL Course");
    Environment env(app);
    Model robot(asset(app, "models/robot.gltf"));
    Mesh ball(sphere()), box(cube()), plane(quad(), quadIndices());
    Camera camera;
    camera.pos = {0, 1, 10};
    camera.pitch = -6;
    Shader model(app, "model.vert", "pbr.frag"), pbr(app, "advanced.vert", "pbr.frag"),
        sky(app, "sky.vert", "sky.frag"), bright(app, "screen.vert", "bright.frag"),
        blur(app, "screen.vert", "blur.frag"), tone(app, "screen.vert", "tone.frag");
    RenderTarget scene, highlights, ping, pong;
    // 每帧处理输入并绘制；GPU 对象在退出循环后按 C++ 作用域逆序释放。
    while (app.begin()) {
        if (app.width < 1 || app.height < 1) {
            app.end();
            continue;
        }
        camera.update(app);
        scene.resize(app.width, app.height);
        highlights.resize(app.width, app.height);
        ping.resize(app.width, app.height);
        pong.resize(app.width, app.height);
        scene.bind();
        glEnable(GL_DEPTH_TEST);
        app.clear();
        env.bindIBL();
```

### Vulkan 原生 · 已实现

底层渲染器接收借用上下文，不负责创建窗口/交换链。

[Vulkan/native/lessons/vk09_pbr/support/scene_vk.hpp](../../Vulkan/native/lessons/vk09_pbr/support/scene_vk.hpp#L198)，第 198—225 行：

```cpp
class SceneVk {
public:
    VkContextRef c;
    int mode;
    VkFormat colorFormat;
    Geometry mesh;
    VkBufferResource vertices, indices, uniform;
    VkImageResource texture;
    VkSampler sampler{};
    VkDescriptorSetLayout setLayout{};
    VkDescriptorPool descriptorPool{};
    VkDescriptorSet descriptor{};
    VkPipelineLayout pipelineLayout{};
    VkPipeline pipeline{};
    VkBuffer overrideVertices{};
    VkBuffer indirectArguments{};
    bool halfScissor = false;
    bool halfViewport = false;
    std::optional<Frame> overrideFrame;
    std::function<Frame(float, float, bool)> frameProvider;
    std::vector<Draw> overrideDraws;
    std::shared_ptr<void> extension;
    SceneVk(const VkContextRef& context,
            int lesson,
            VkFormat format,
            const std::filesystem::path& shaders,
            const Geometry* replacement = nullptr,
            VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT,
```

### Vulkan / nvpro_core2 · 已实现

生命周期回调管理自定义资源；不是第四个图形 API。

[Vulkan/nvpro/lessons/nv06_pbr_viewport/support/nvapp_course.hpp](../../Vulkan/nvpro/lessons/nv06_pbr_viewport/support/nvapp_course.hpp#L44)，第 44—78 行：

```cpp
class NvLesson : public nvapp::IAppElement {
    nvapp::Application* app{};
    VkContextRef c;
    nvvk::ResourceAllocator allocator;
    nvvk::RenderTarget target;
    nvapp::ImTexture display;
    std::unique_ptr<SceneVk> scene;
    nvvk::Buffer uploadedVertices;
    int mode;
    bool smoke;
    float roughnessScale = 1;
    std::filesystem::path output;

public:
    NvLesson(int m, bool test, const std::filesystem::path& path)
        : mode(m), smoke(test), output(path) {
    }
    void onAttach(nvapp::Application* application) override {
        app = application;
        auto queue = app->getQueue(0);
        c = {app->getInstance(),
             app->getPhysicalDevice(),
             app->getDevice(),
             queue.queue,
             queue.familyIndex};

        // VMA 管理分配与绑定，调用方仍需声明用途并保证 GPU 使用期间资源存活。
        VmaAllocatorCreateInfo ai{};
        ai.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
        ai.physicalDevice = c.physical;
        ai.device = c.device;
        ai.instance = c.instance;
        ai.vulkanApiVersion = VK_API_VERSION_1_4;
        vkCheck(allocator.init(ai));
        nvvk::RenderTarget::CreateInfo info{};
```

### DirectX 12 · 已实现

独立 DxScene/DxWindow 先可解释，再考虑抽象统一 Renderer。

[DirectX12/lessons/dx09_pbr/support/dx12_course.hpp](../../DirectX12/lessons/dx09_pbr/support/dx12_course.hpp#L245)，第 245—269 行：

```cpp
class DxScene {
public:
    DxDevice& c;
    int mode;
    Geometry mesh;
    ComPtr<ID3D12Resource> vertices, indices, uniform, texture;
    ComPtr<ID3D12DescriptorHeap> srvHeap;
    ComPtr<ID3D12RootSignature> signature;
    ComPtr<ID3D12PipelineState> pipeline;
    D3D12_VERTEX_BUFFER_VIEW vb{};
    D3D12_INDEX_BUFFER_VIEW ib{};
    std::optional<Frame> overrideFrame;
    std::function<Frame(float, float, bool)> frameProvider;
    std::vector<Draw> overrideDraws;
    bool halfScissor = false;
    bool halfViewport = false;
    ComPtr<ID3D12Resource> indirectArguments;
    ComPtr<ID3D12CommandSignature> indirectSignature;
    std::shared_ptr<void> extension;
    DxScene(DxDevice& context,
            int lesson,
            const Geometry* replacement = nullptr,
            UINT samples = 1,
            D3D12_CULL_MODE cull = D3D12_CULL_MODE_NONE,
            bool clampSampler = false,
```
