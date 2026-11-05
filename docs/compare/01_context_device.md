# 01 · 窗口、上下文、设备和队列

[对照路线](../COMPARE.md) · [交互式对照](index.html#01_context_device) · [上一章](00_learning_path.md) · [下一章](02_triangle_pipeline.md)

本章解决“为什么 OpenGL 创建窗口后就能调用绘图函数，而 Vulkan/DX12 还需要很多对象”。目标是能画出初始化依赖图，并区分窗口管理与 GPU 工作管理。

## 窗口不等于渲染设备

操作系统窗口提供可见区域和输入事件。OpenGL context 保存当前程序、VAO、纹理等状态；本仓库让 GLFW 同时创建窗口和 context。Vulkan 路线先要求 GLFW 不创建 OpenGL context，再创建 instance、surface 和逻辑设备。DX12 也使用无 OpenGL context 的窗口，通过原生 HWND 把 DXGI 交换链连接到它。

| 问题 | OpenGL | Vulkan 原生 | DX12 |
| --- | --- | --- | --- |
| 谁管理窗口和键盘鼠标？ | GLFW | GLFW | GLFW / HWND |
| 谁决定可用功能？ | context 版本与扩展 | 物理设备查询、特性启用 | adapter、feature level、feature queries |
| 谁接收 GPU 工作？ | 当前 context 对应的驱动路径 | 创建的 VkQueue | ID3D12CommandQueue |
| 谁连接窗口与呈现？ | context 的窗口系统绑定 | SurfaceKHR 与 SwapchainKHR | DXGI swap chain |

## 顺着代码走一遍

OpenGL 的顺序是 `glfwInit → hints → glfwCreateWindow → glfwMakeContextCurrent → gladLoadGL`。GLAD 加载的是函数入口；没有正确的当前 context，找到函数地址也不代表可以合法使用对应功能。GPU 对象必须在 context 仍然存在时销毁，因此示例先创建 App，再创建 Shader 和 Mesh，利用局部对象的反向析构顺序。

原生 Vulkan 的 `NativeDevice` 先列出窗口系统所需实例扩展，然后创建实例和可选 debug messenger。之后枚举物理设备，检查版本、dynamicRendering、synchronization2，以及队列族是否同时支持 graphics/compute；有窗口时还检查该 surface 的呈现支持。此代码采用同一队列族以减少所有权转移的复杂度，不保证是所有设备上的最优队列选择。

查询到一个特性与启用一个特性是两件事。前者回答驱动能不能做，后者是在创建设备时声明本程序要使用什么。`pNext` 把多个版本/扩展的特性结构串成链；不要把查询用的结构和创建时要启用的内容当作没有差别的清单。

DX12 的 `DxDevice` 先尝试开启调试层，再创建 factory、选择 adapter、创建设备和 direct queue。`--warp` 改用软件 adapter，可帮助区分硬件相关问题，但软件执行的帧率不能用于比较显卡性能。feature level 12_0 是本课程门槛，不是“所有 DX12 新功能都支持”的证明。

## nvpro_core2 的作用

`nvvk::Context` 接收实例扩展、设备扩展、特性和队列要求并完成创建。应用仍需声明需求、检查初始化结果和保证拥有者活得足够久。框架内部使用的 Vulkan 对象仍受同样的生命周期规则约束；“被框架封装”不意味着资源不再依赖 device。

## 动手实验与答案

1. 分别运行 OpenGL 窗口课、vk01、nv01 和 dx01，记录设备名、API 请求版本以及是否真的创建窗口。预期只有设备检查课可以没有图像；这是其验证目标，不是黑屏错误。
2. 在原生 Vulkan 设备筛选位置打断点，记录选中 queue family 的 flags 和 present support。预测：把 graphics 条件删除并不能让 graphics pipeline 在任意队列上合法运行。
3. 画销毁关系。参考答案：先停止 GPU 使用，再释放依赖 device 的资源和管线，最后销毁 device；窗口 surface 与实例也要按依赖顺序处理。C++ 的析构只处理 CPU 控制流，不会自动等待未完成的 GPU 任务。

## 常见错误

初始化就失败时先看 GLFW/loader 错误，再看扩展、验证层和功能查询。链接器找不到库与运行时设备不满足特性属于两个阶段，不能靠反复复制 DLL 混着解决。[GLFW Vulkan guide](https://www.glfw.org/docs/latest/vulkan_guide.html) 可用于核对窗口与 surface 的边界。

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

先 current context，再加载 GL 入口；GPU 对象先于 context 销毁。

[OpenGL/lessons/01_window/support/app.hpp](../../OpenGL/lessons/01_window/support/app.hpp#L25)，第 25—77 行：

```cpp
    App(int argc, char** argv, const char* title) {
        for (int i = 1; i < argc; ++i) {
            smoke |= std::string(argv[i]) == "--smoke";
            capture |= std::string(argv[i]) == "--capture";
            variant |= std::string(argv[i]) == "--variant";
        }
        expectGeometry = std::string(title).find("01_window") != 0;
        assets = std::filesystem::absolute(argv[0]).parent_path() / COURSE_RUNTIME_NAME / "shaders";
        glfwSetErrorCallback([](int c, const char* s) {
            std::cerr << "GLFW " << c << ": " << s << '\n';
        });
        if (!glfwInit())
            throw std::runtime_error("GLFW initialization failed");
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
        if (smoke)
            glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
        window = glfwCreateWindow(width, height, title, nullptr, nullptr);
        if (!window) {
            glfwTerminate();
            throw std::runtime_error("Cannot create OpenGL 3.3 core context");
        }

        // 必须先让上下文在当前线程生效，再由 GLAD 加载 OpenGL 函数入口。
        glfwMakeContextCurrent(window);
        if (!gladLoadGL(glfwGetProcAddress)) {
            glfwDestroyWindow(window);
            glfwTerminate();
            throw std::runtime_error("GLAD load failed");
        }
        glfwSwapInterval(smoke ? 0 : 1);
        std::cout << "GPU: " << glGetString(GL_RENDERER) << "\nOpenGL: " << glGetString(GL_VERSION)
                  << '\n';
        last = float(glfwGetTime());
    }
    ~App() {
        glfwDestroyWindow(window);
        glfwTerminate();
    }
    App(const App&) = delete;
    App& operator=(const App&) = delete;
    int key(int k) const {
        return smoke && variant && k == GLFW_KEY_SPACE ? GLFW_PRESS : glfwGetKey(window, k);
    }
    float time() const {
        return smoke ? 1.0f : float(glfwGetTime());
    }

    // 每帧先处理事件并读取 framebuffer 尺寸；高 DPI 下它可能不同于窗口逻辑尺寸。
```

### Vulkan 原生 · 已实现

扩展属于实例或设备，队列族还要检查实际呈现支持。

[Vulkan/native/lessons/vk01_instance_device/support/native_app.hpp](../../Vulkan/native/lessons/vk01_instance_device/support/native_app.hpp#L29)，第 29—70 行：

```cpp
    NativeDevice(GLFWwindow* window, bool timeline = false) {
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
        di.pfnUserCallback = validation;
        VkInstanceCreateInfo info{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
        info.pApplicationInfo = &app;
        info.enabledExtensionCount = uint32_t(extensions.size());
        info.ppEnabledExtensionNames = extensions.data();
        if (debug) {
            info.enabledLayerCount = 1;
            info.ppEnabledLayerNames = &layer;
            info.pNext = &di;
        }
        vkCheck(vkCreateInstance(&info, nullptr, &c.instance));
        auto create = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
            vkGetInstanceProcAddr(c.instance, "vkCreateDebugUtilsMessengerEXT"));
        if (debug && create)
```

### Vulkan / nvpro_core2 · 已实现

Context 减少创建代码，需求选择与失败处理仍由应用负责。

[Vulkan/nvpro/lessons/nv01_context/main.cpp](../../Vulkan/nvpro/lessons/nv01_context/main.cpp#L15)，第 15—46 行：

```cpp
    nvvk::ContextInitInfo setup{};
    setup.instanceExtensions = {VK_EXT_DEBUG_UTILS_EXTENSION_NAME};
    setup.queues = {VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT};
    setup.enableValidationLayers = true;
    setup.verbose = false;
    setup.applicationName = "nvpro_core2 Course";
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
                       context.getPhysicalDevice(),
                       context.getDevice(),
                       q.queue,
                       q.familyIndex};
        {
            VkPhysicalDeviceProperties p;
            vkGetPhysicalDeviceProperties(c.physical, &p);
            std::cout << "nvpro_core2 context: " << p.deviceName << '\n';
        }
    }
    std::cout << "Validation errors: " << nvValidationErrors << "\n";
    return nvValidationErrors ? 1 : 0;
}
} // namespace course

```

### DirectX 12 · 已实现

在设备创建前启用调试层；WARP 是软件参考路径。

[DirectX12/lessons/dx01_device/support/dx12_course.hpp](../../DirectX12/lessons/dx01_device/support/dx12_course.hpp#L74)，第 74—115 行：

```cpp
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
                adapter->GetDesc1(&desc);
                if (!(desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) &&
                    SUCCEEDED(D3D12CreateDevice(
                        adapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&device))))
                    break;
                adapter.Reset();
            }
            if (!device)
                throw std::runtime_error("No D3D12 feature level 12_0 adapter; try --warp");
        }
        DXGI_ADAPTER_DESC1 desc;
        adapter->GetDesc1(&desc);
        std::wcout << L"GPU: " << desc.Description << L'\n';
        device.As(&messages);
        D3D12_COMMAND_QUEUE_DESC qi{};
        qi.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        dxCheck(device->CreateCommandQueue(&qi, IID_PPV_ARGS(&queue)));
        dxCheck(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
                                               IID_PPV_ARGS(&allocator)));
        dxCheck(device->CreateCommandList(
```
