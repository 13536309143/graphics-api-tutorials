# nv03_triangle · 动态渲染与共享场景

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
cmake --build --preset vulkan-nvpro-debug --target nv03_triangle --parallel 4
.\Vulkan\nvpro\build\bin\Debug\nv03_triangle.exe
```

以上命令从仓库根目录执行，也可使用本页“运行本课”。预期：Viewport 显示彩色三角形。

## 代码阅读

先读 [main.cpp](main.cpp)，再读路线根目录 common/nvapp_course.hpp 的 NvLesson/nvproMain。框架实现位于 external/nvpro_core2；其固定 commit 和许可证见路线 README。共享底层实现位于 Vulkan/common/scene_vk.hpp，公共数学/几何位于 shared/scene.hpp。

nvvk::RenderTargetState 把附件状态整理为动态渲染信息，cmdBeginRendering 开始 pass；底层图形 pipeline 与 shader 仍通过 Vulkan 代码构建。此处刻意与 native 共用 SceneVk，方便把“框架负责窗口/目标”和“应用仍负责 shader/绘制状态”区分开。GENERAL 布局不代表可以省略图像写后读屏障。

## 动手练习

读 RenderTargetState::cmdBeginRendering，对照原生 VkRenderingInfo；把 Viewport 内容保存并核对读回。

自测：列出本课中由 nvpro_core2 管理的对象，以及仍由 SceneVk/应用管理的对象。参考：Context/Application/RenderTarget/Allocator 负责各自对象，shader、pipeline layout、descriptor 与绘制逻辑仍有显式 Vulkan 状态；框架不会自动推断任意自定义 pass 的全部依赖。

## 检查

`nv03_triangle.exe --smoke` 使用 GPU 无窗口运行或控制台检查。渲染课读回 Viewport 图像为 PPM；计算课逐项验证。验证层错误会使测试失败。常规窗口的 resize/minimize、UI 输入和长时间运行需要另外检查，不能以 headless 通过代替全部交互验证。


## 实测画面

固定时间的 GPU 读回图像（nvpro 为 headless Viewport 输出）。

![nv03_triangle](../../../../docs/previews/nv03_triangle.png)


## 深入学习：数据、接口与执行

onRender 用 RenderTargetState 开始动态渲染，SceneVk 录制三角形，再把颜色写依赖连接到 UI 采样。GENERAL 布局保留也不消除这条写后读关系。

配套精讲：[三 API 对照 · 三角形、管线与着色器](../../../../docs/compare/02_triangle_pipeline.md)。先读概念与参数选择，再返回本课实现核对，不必一次展开所有公共类。

### 本课的真实源码切片

来自 [Vulkan/nvpro/lessons/nv03_triangle/main.cpp](main.cpp#L5)，第 5—60 行；这是现有实现的局部摘录，末尾可能处于函数中间，不是可单独编译的完整程序。

```cpp
int runChapter(int argc, char** argv) {
    bool smoke = false;
    for (int i = 1; i < argc; ++i)
        smoke |= std::strcmp(argv[i], "--smoke") == 0;
    // 课程编号与公共场景模式分别管理：第 4 课看纹理上传，第 6 课看 PBR。
    int mode = ((3));
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
    if (!smoke && 3 != 1 && 3 != 5) {
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
                                                          ("nvpro_" + std::to_string(3) + ".ppm"));
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
| OpenGL | [02_triangle](../../../../OpenGL/lessons/02_triangle/README.md) |
| Vulkan 原生 | [vk03_triangle](../../../native/lessons/vk03_triangle/README.md) |
| Vulkan / nvpro_core2 | [nv03_triangle](README.md) |
| DirectX 12 | [dx03_triangle](../../../../DirectX12/lessons/dx03_triangle/README.md) |

### 先预测，再验证

省略最后屏障，某驱动仍显示正常能证明安全吗？

参考推导：不能。偶然顺序不等于合法依赖；应按消费者阶段和访问推导，并用同步验证辅助检查。

在一次实验中只改一个因素，记录预测、实际画面/日志和原因。检查已有 smoke 是否仍通过；图形调试器里再核对本次 draw 的实际输入/输出，不能仅凭进程正常退出下结论。
