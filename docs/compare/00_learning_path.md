# 00 · 如何用同一套问题学习三种 API

[对照路线](../COMPARE.md) · [交互式对照](index.html#00_learning_path) · [下一章](01_context_device.md)

这条对照路线面向“OpenGL 画得出来，但 Vulkan/DX12 只会照抄初始化”的阶段。每章先给出要解决的问题，再解释三种 API 把责任放在哪里，最后回到本仓库的真实实现和可复现实验。nvpro_core2 始终作为 Vulkan 的框架路线单独观察，它不是第四种图形 API。

## 先建立一张工作清单

一帧图像需要解决六件事：准备输入数据、声明着色器接口、配置管线、选择输出目标、提交 GPU 工作、保证结果在被读取前已经准备好。换 API 时，这些工作仍然存在，改变的是对象划分、状态保存位置和同步责任。

不要一开始就背 `Vk*CreateInfo` 的所有字段。先为每个对象回答：谁创建、谁拥有、谁使用、依赖什么、什么时候允许销毁。例如 image view 不拥有图片像素，descriptor 也不会复制纹理；即使 C++ 函数已经返回，GPU 仍可能访问它们。

| 学习阶段 | 本阶段产物 | 必须能解释的问题 |
| --- | --- | --- |
| 环境与窗口 | 能清屏、能退出、能打印设备信息 | 窗口、context、device、surface 有何不同？ |
| 三角形与几何 | 能解释每个顶点来自哪里 | 顶点输入与 descriptor 为什么是两类接口？ |
| 常量与纹理 | 能追踪 CPU 数据到 shader | 数据、资源、view、绑定分别在哪里？ |
| 深度与同步 | 能画出正确遮挡并读回 | 谁写谁读，CPU/GPU 分别等待什么？ |
| 模型与材质 | 能定位导入、变换和着色责任 | 模型格式、BRDF 和 API 哪些可以共享？ |
| 多遍与工程 | 能画依赖图并定位错误 | 改为两帧并行后，哪些资源必须分开？ |

## 本工程的基线不能混在一起

OpenGL 使用 3.3 Core 和 GLSL 330。原生 Vulkan 使用 C API、Vulkan 1.3、GLSL/SPIR-V、dynamic rendering 和 synchronization2。nvpro 路线使用固定提交的 nvpro_core2，按其要求使用 Vulkan 1.4。DX12 使用传统 resource barriers、D3DCompiler 和 Shader Model 5.1。这些是当前代码的选择，不代表各 API 的全部能力。

你提供的 Vulkan 中文镜像介绍了原始 C API 路线；当前官方 `latest` 已采用不同的基线与工具组合。学习外部代码时先确认 Vulkan 版本、C API/Vulkan-Hpp、GLSL/Slang、render pass/dynamic rendering、binary/timeline semaphore 五项，不能只看章节标题相同就拼接实现。详见[参考资料与版本差异](../REFERENCES.md)。

## 两种阅读顺序

第一次学图形时，先完成 OpenGL 01—10，建立坐标、插值、纹理和光照直觉，再按本对照路线学习显式 API。已有 OpenGL 基础时，每章同时打开三个实现，以“同一份顶点如何到达屏幕”为线索，不必把 34 个 OpenGL 高级效果全部做完再进入 Vulkan。

每次实验写四行记录：修改前的预测、只改的一个变量、画面/日志证据、对预测的修正。遇到黑屏先退回固定颜色三角形；不要同时改矩阵、剔除和纹理绑定，否则无法知道哪个修改起作用。

## 完成标准

能画出正确图片只完成了一半。你还应能在源码中找到创建、更新、绑定、提交和释放位置；说明图像为什么正确；用一个受控修改改变结果；通过日志或抓帧证明自己的解释。程序无验证错误是必要证据，但不证明光照公式或相机坐标正确。

本次新增内容主要加深讲解、源码对照和实验设计。当前仍有 60 个可执行课程；模型/IBL/SSAO 等只在 OpenGL 完成的功能会明确标为“迁移设计”，不会因为有理论章节就算成新的 Vulkan/DX12 实现。

<!-- generated-source-comparison -->

## 与本仓库实现并排核对

以下切片直接取自当前仓库，省略了前后文。行号用于定位，不能把片段当作完整可编译程序。

| 路线 | 可运行参照 / 范围 |
| --- | --- |
| OpenGL | [01_window](../../OpenGL/lessons/01_window/README.md) |
| Vulkan 原生 | [vk01_instance_device](../../Vulkan/native/lessons/vk01_instance_device/README.md) |
| Vulkan / nvpro_core2 | [nv01_context](../../Vulkan/nvpro/lessons/nv01_context/README.md) |
| DirectX 12 | [dx01_device](../../DirectX12/lessons/dx01_device/README.md) |

### OpenGL · 已实现

先读最短清屏路径，建立 CPU 帧循环。

[OpenGL/lessons/01_window/main.cpp](../../OpenGL/lessons/01_window/main.cpp#L5)，第 5—15 行：

```cpp
int main(int argc, char** argv) try {
    App app(argc, argv, "01_window - OpenGL Course");
    // 渲染循环 = 处理事件 → 设置视口 → 清屏 → 交换前后缓冲。
    while (app.begin()) {
        app.clear();
        app.end();
    }
    return 0;
} catch (const std::exception& e) {
    return failure(e);
}
```

### Vulkan 原生 · 已实现

从 instance、设备筛选和队列看显式初始化。

[Vulkan/native/common/native_app.hpp](../../Vulkan/native/common/native_app.hpp#L25)，第 25—56 行：

```cpp
struct NativeDevice {
    VkContextRef c;
    VkDebugUtilsMessengerEXT messenger{};
    VkSurfaceKHR surface{};
    NativeDevice(GLFWwindow* window) {
        uint32_t count = 0;
        std::vector<const char*> extensions;
        if (window) {
            const char** required = glfwGetRequiredInstanceExtensions(&count);
            extensions.assign(required, required + count);
        }
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        uint32_t layerCount = 0;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
        std::vector<VkLayerProperties> layers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, layers.data());
        bool debug = std::any_of(layers.begin(), layers.end(), [](const auto& x) {
            return std::strcmp(x.layerName, "VK_LAYER_KHRONOS_validation") == 0;
        });
        const char* layer = "VK_LAYER_KHRONOS_validation";
        if (!debug)
            std::cerr << "Validation layer not installed; rendering remains available\n";
        VkApplicationInfo app{VK_STRUCTURE_TYPE_APPLICATION_INFO};
        app.pApplicationName = "Native Vulkan Course";
        app.apiVersion = VK_API_VERSION_1_3;
        VkDebugUtilsMessengerCreateInfoEXT di{
            VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
        di.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
        di.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                         VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                         VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
```

### Vulkan / nvpro_core2 · 已实现

框架路线仍需声明特性和队列要求。

[Vulkan/nvpro/common/nvapp_course.hpp](../../Vulkan/nvpro/common/nvapp_course.hpp#L234)，第 234—265 行：

```cpp
inline int nvproMain(int argc, char** argv, int chapter) {
    bool smoke = false;
    for (int i = 1; i < argc; ++i)
        smoke |= std::strcmp(argv[i], "--smoke") == 0;

    // 课程编号与公共场景模式分别管理：第 4 课看纹理上传，第 6 课看 PBR。
    int mode = chapter == 4 ? 6 : chapter == 6 ? 9 : chapter;
    nvShaders = std::filesystem::absolute(argv[0]).parent_path() / "shaders";
    NvContextOwner owner;
    auto& context = owner.context;
    nvValidationErrors = 0;
    nvvk::ContextInitInfo setup{};
    setup.instanceExtensions = {VK_EXT_DEBUG_UTILS_EXTENSION_NAME};
    setup.queues = {VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT};
    setup.enableValidationLayers = true;
    setup.verbose = false;
    setup.applicationName = "nvpro_core2 Course";
    if (!smoke && chapter != 1 && chapter != 5) {
        nvvk::addSurfaceExtensions(setup.instanceExtensions);
        setup.deviceExtensions = {{VK_KHR_SWAPCHAIN_EXTENSION_NAME}};
    }
    vkCheck(context.init(setup));
    VkDebugUtilsMessengerCreateInfoEXT debug{
        VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
    debug.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    debug.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
    debug.pfnUserCallback = nvValidation;
    vkCheck(
        vkCreateDebugUtilsMessengerEXT(context.getInstance(), &debug, nullptr, &owner.messenger));
    {
        auto q = context.getQueueInfo(0);
        VkContextRef c{context.getInstance(),
```

### DirectX 12 · 已实现

从 adapter、device、queue 看设备建立。

[DirectX12/common/dx12_course.hpp](../../DirectX12/common/dx12_course.hpp#L59)，第 59—90 行：

```cpp
struct DxDevice {
    ComPtr<IDXGIFactory6> factory;
    ComPtr<ID3D12Device> device;
    ComPtr<ID3D12CommandQueue> queue;
    ComPtr<ID3D12CommandAllocator> allocator;
    ComPtr<ID3D12GraphicsCommandList> cmd;
    ComPtr<ID3D12Fence> fence;
    ComPtr<ID3D12InfoQueue> messages;
    HANDLE event = nullptr;
    UINT64 value = 0;
    explicit DxDevice(bool warp = false) {
        UINT flags = 0;
        ComPtr<ID3D12Debug> debug;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug)))) {
            debug->EnableDebugLayer();
            flags |= DXGI_CREATE_FACTORY_DEBUG;
            std::cout << "D3D12 debug layer enabled\n";
        } else
            std::cout
                << "D3D12 debug layer unavailable; install Windows Graphics Tools for validation\n";
        dxCheck(CreateDXGIFactory2(flags, IID_PPV_ARGS(&factory)));
        ComPtr<IDXGIAdapter1> adapter;
        if (warp) {
            dxCheck(factory->EnumWarpAdapter(IID_PPV_ARGS(&adapter)));
            dxCheck(
                D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&device)));
        } else {
            for (UINT i = 0; factory->EnumAdapterByGpuPreference(
                                 i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&adapter)) !=
                             DXGI_ERROR_NOT_FOUND;
                 ++i) {
                DXGI_ADAPTER_DESC1 desc;
```
