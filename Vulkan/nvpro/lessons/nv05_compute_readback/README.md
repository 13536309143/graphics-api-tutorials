# nv05_compute_readback · Compute 与读回验证

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
cmake --build --preset vulkan-nvpro-debug --target nv05_compute_readback --parallel 4
.\Vulkan\nvpro\build\bin\Debug\nv05_compute_readback.exe
```

以上命令从仓库根目录执行，也可使用本页“运行本课”。预期：无窗口，在 context 队列完成 256 项计算并验证。

## 代码阅读

先读 [main.cpp](main.cpp)，再读路线根目录 common/nvapp_course.hpp 的 NvLesson/nvproMain。框架实现位于 external/nvpro_core2；其固定 commit 和许可证见路线 README。共享底层实现位于 Vulkan/common/scene_vk.hpp，公共数学/几何位于 shared/scene.hpp。

在 nvpro 管理的 device/queue 上调用与原生课程相同的 computeSquares。这样可逐项验证 framework context 没有改变 shader binding、dispatch 数量和 host 可见性规则。当前保留原生 SSBO/descriptor/pipeline 代码，以便清楚比较上下文封装前后的边界；不是把一段 CPU 运算包装成框架 compute。

## 动手练习

把存储/读回缓冲改为 VMA mapped buffer，保留 flush/invalidate 和 fence 正确性。

自测：列出本课中由 nvpro_core2 管理的对象，以及仍由 SceneVk/应用管理的对象。参考：Context/Application/RenderTarget/Allocator 负责各自对象，shader、pipeline layout、descriptor 与绘制逻辑仍有显式 Vulkan 状态；框架不会自动推断任意自定义 pass 的全部依赖。

## 检查

`nv05_compute_readback.exe --smoke` 使用 GPU 无窗口运行或控制台检查。渲染课读回 Viewport 图像为 PPM；计算课逐项验证。验证层错误会使测试失败。常规窗口的 resize/minimize、UI 输入和长时间运行需要另外检查，不能以 headless 通过代替全部交互验证。


## 深入学习：数据、接口与执行

此课没有常规视口循环，Context 提供设备/队列，复用 computeSquares 完成 GPU 计算和逐项检查。共享算法让你单独观察框架初始化差异。

配套精讲：[三 API 对照 · 计算与读回](../../../../docs/compare/10_compute_readback.md)。先读概念与参数选择，再返回本课实现核对，不必一次展开所有公共类。

### 本课的真实源码切片

来自 [Vulkan/nvpro/lessons/nv05_compute_readback/main.cpp](main.cpp#L5)，第 5—50 行；这是现有实现的局部摘录，末尾可能处于函数中间，不是可单独编译的完整程序。

```cpp
int runChapter(int argc, char** argv) {
    bool smoke = false;
    for (int i = 1; i < argc; ++i)
        smoke |= std::strcmp(argv[i], "--smoke") == 0;
    // 课程编号与公共场景模式分别管理：第 4 课看纹理上传，第 6 课看 PBR。
    int mode = ((5));
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
            computeSquares(c, nvShaders);
        }
    }
    std::cout << "Validation errors: " << nvValidationErrors << "\n";
    return nvValidationErrors ? 1 : 0;
}
} // namespace course

int main(int argc, char** argv) try {
    return course::runChapter(argc, argv);
} catch (const std::exception& error) {
    std::cerr << "ERROR: " << error.what() << '\n';
    return 1;
}
```

### 同主题跨 API 导航

以下是同主题参考；场景、功能完成度和版本基线可能不同，具体边界见对照章节。

| 路线 | 可运行参照 / 当前范围 |
| --- | --- |
| OpenGL | 基线外：当前 OpenGL 3.3 路线没有 compute 实现；粒子实例化不能作为 compute 对照。 |
| Vulkan 原生 | [vk08_compute_readback](../../../native/lessons/vk08_compute_readback/README.md) |
| Vulkan / nvpro_core2 | [nv05_compute_readback](README.md) |
| DirectX 12 | [dx08_compute_uav](../../../../DirectX12/lessons/dx08_compute_uav/README.md) |

### 先预测，再验证

没有弹窗却退出零，如何证明真的用了 GPU？

参考推导：查看 pipeline/dispatch/submit 路径与 256 项验证输出；不能仅以控制台是否打印设备名判断。

在一次实验中只改一个因素，记录预测、实际画面/日志和原因。检查已有 smoke 是否仍通过；图形调试器里再核对本次 draw 的实际输入/输出，不能仅凭进程正常退出下结论。
