# nv02_application · nvapp Application 生命周期

## 本课独立工程与关键注释

[main.cpp](main.cpp) · [本地辅助实现](support/) · [本课着色器](shaders/) · [CMakeLists.txt](CMakeLists.txt)

本目录拥有完整课程源码、资源与依赖配方。关键步骤含中文注释，不链接其他课的课程实现。可将整个目录复制到其他位置，用以下命令单独构建和检查；第三方依赖首次配置需要联网或提供缓存。

```powershell
cmake -S . -B build -A x64
cmake --build build --config Debug --parallel 4
ctest --test-dir build -C Debug --output-on-failure
```

[路线首页](../../README.md) · [原生 Vulkan 对照](../../../native/README.md)

## 构建与运行

```powershell
cmake --preset vulkan-nvpro
cmake --build --preset vulkan-nvpro-debug --target nv02_application --parallel 4
.\Vulkan\nvpro\build\bin\Debug\nv02_application.exe
```

以上命令从仓库根目录执行，也可使用本页“运行本课”。预期：深蓝色 Viewport，可调窗口布局。

## 代码阅读

先读 [main.cpp](main.cpp)，再读路线根目录 common/nvapp_course.hpp 的 NvLesson/nvproMain。框架实现位于 external/nvpro_core2；其固定 commit 和许可证见路线 README。共享底层实现位于 Vulkan/common/scene_vk.hpp，公共数学/几何位于 shared/scene.hpp。

nvapp::Application 管理窗口、交换链、UI 和帧提交，IAppElement 提供 onAttach/onResize/onUIRender/onRender/onDetach。渲染不直接写交换链，而是写 nvvk::RenderTarget，再交给 ImGui 显示。onResize 中更新附件与显示描述符；资源必须先于 allocator/device 销毁。--smoke 使用官方 headless 路径，仍进行真实 GPU 渲染。

## 动手练习

修改清屏色；在每个回调中打印一次调用顺序；缩放 Viewport 观察 update。

自测：列出本课中由 nvpro_core2 管理的对象，以及仍由 SceneVk/应用管理的对象。参考：Context/Application/RenderTarget/Allocator 负责各自对象，shader、pipeline layout、descriptor 与绘制逻辑仍有显式 Vulkan 状态；框架不会自动推断任意自定义 pass 的全部依赖。

## 检查

`nv02_application.exe --smoke` 使用 GPU 无窗口运行或控制台检查。渲染课读回 Viewport 图像为 PPM；计算课逐项验证。验证层错误会使测试失败。常规窗口的 resize/minimize、UI 输入和长时间运行需要另外检查，不能以 headless 通过代替全部交互验证。


## 实测画面

固定时间的 GPU 读回图像（nvpro 为 headless Viewport 输出）。

![nv02_application](../../../../docs/previews/nv02_application.png)


## 深入学习：数据、接口与执行

Application 驱动回调，RenderTarget 提供离屏附件，ImTexture 将视图连接到 UI。尺寸重建后显示 view 必须更新，headless 检查则不创建普通交互界面。

配套精讲：[三 API 对照 · 目标、深度与 MSAA](../../../../docs/compare/07_targets_depth_msaa.md)。先读概念与参数选择，再返回本课实现核对，不必一次展开所有公共类。

### 本课的真实源码切片

来自 [Vulkan/nvpro/lessons/nv02_application/main.cpp](main.cpp#L5)，第 5—60 行；这是现有实现的局部摘录，末尾可能处于函数中间，不是可单独编译的完整程序。

```cpp
int runChapter(int argc, char** argv) {
    bool smoke = false;
    for (int i = 1; i < argc; ++i)
        smoke |= std::strcmp(argv[i], "--smoke") == 0;
    // 课程编号与公共场景模式分别管理：第 4 课看纹理上传，第 6 课看 PBR。
    int mode = ((2));
    nvShaders = std::filesystem::absolute(argv[0]).parent_path() / COURSE_RUNTIME_NAME / "shaders";
    NvContextOwner owner;
    auto& context = owner.context;
    nvValidationErrors = 0;
    nvvk::ContextInitInfo setup{};
    setup.instanceExtensions = {VK_EXT_DEBUG_UTILS_EXTENSION_NAME};
    setup.queues = {VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT};
    setup.enableValidationLayers = true;
    setup.verbose = false;
    setup.applicationName = "nvpro_core2 Course";
    if (!smoke && 2 != 1 && 2 != 5) {
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
                       context.getPhysicalDevice(),
                       context.getDevice(),
                       q.queue,
                       q.familyIndex};
        {
            nvapp::ApplicationCreateInfo info;
            info.name = "nvpro_core2 Course";
            info.instance = c.instance;
            info.device = c.device;
            info.physicalDevice = c.physical;
            info.queues = context.getQueueInfos();
            info.windowSize = {960, 640};
            info.headless = smoke;
            info.headlessFrameCount = 4;
            nvapp::Application app;
            app.init(info);
            auto element = std::make_shared<NvLesson>(mode,
                                                      smoke,
                                                      std::filesystem::path(argv[0]).parent_path() /
                                                          ("nvpro_" + std::to_string(2) + ".ppm"));
            app.addElement(element);
            app.run();
            app.deinit();
        }
    }
```

### 同主题跨 API 导航

以下是同主题参考；场景、功能完成度和版本基线可能不同，具体边界见对照章节。

| 路线 | 可运行参照 / 当前范围 |
| --- | --- |
| OpenGL | [29_msaa](../../../../OpenGL/lessons/29_msaa/README.md) |
| Vulkan 原生 | [vk10_offscreen](../../../native/lessons/vk10_offscreen/README.md) |
| Vulkan / nvpro_core2 | [nv02_application](README.md) |
| DirectX 12 | [dx10_offscreen](../../../../DirectX12/lessons/dx10_offscreen/README.md) |

### 先预测，再验证

为什么 UI 可见但场景仍可能没画？

参考推导：UI pass 和场景 pass 是两条工作。应在抓帧中找到场景目标及其 clear/draw，而不是只看最后 ImGui draw。

在一次实验中只改一个因素，记录预测、实际画面/日志和原因。检查已有 smoke 是否仍通过；图形调试器里再核对本次 draw 的实际输入/输出，不能仅凭进程正常退出下结论。
