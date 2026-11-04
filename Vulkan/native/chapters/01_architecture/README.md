# 第 01 课 · 图形 API 的职责与课程地图 · Vulkan 原生

[54 课目录](../../COURSE.md) · [对照精讲](../../../../docs/compare/00_learning_path.md) · [下一课](../02_environment/README.md)

## 本课目标与先修

学习目标：解释“图形 API 的职责与课程地图”解决的问题，找到实际对象和参数，运行对应实验并用数据/画面证明。先了解 C++ 对象作用域和基本向量/矩阵；环境配置在下一课逐项检查。

## 为什么需要这一层

从“一个模型最终成为像素”倒推系统：资产只提供 CPU 数据，资源承载 GPU 可访问字节，管线解释这些字节，命令描述操作，队列异步执行，呈现把结果交给窗口系统。学习时为每个对象记录创建者、使用者与最后使用时间，才能把长初始化放回整体位置。

本主线以官方顺序建立依赖，但保留原有实验编号与链接。教材章号是学习次序，vk/dx 实验号是稳定入口，二者不要求相等。每后端有 54 章和 54 个独立程序；设备查询、计算和图形程序各有不同证据，不用一张图片证明所有内容。

## Vulkan 原生 的对象、参数与执行推导

Instance 管理应用与实现的连接；PhysicalDevice 描述能力；Device 启用选择的能力；Queue 接收提交。本工程用 Vulkan 1.3 C API、dynamic rendering 和 sync2。参考教程中传统 RenderPass/Framebuffer 会对照解释，本工程不因此伪称使用了那套对象。

## 对照 DirectX 12：相同问题的另一种分工

DXGI factory/adapter 处理平台与设备选择，ID3D12Device 创建资源与管线，CommandQueue 执行命令。root signature 描述资源接口，PSO 描述着色器与固定状态；它们不等价于整套 Renderer 或操作系统窗口。

## 关键参数与可检查不变量

| 检查项 | Vulkan 表达 | DX12 表达 | 不变量 / 证据 |
| --- | --- | --- | --- |
| 职责边界 | Instance/Device/Queue | Factory/Device/CommandQueue | 对象存在不代表已提交或已完成 |
| 资源与引用 | Buffer/Image 与 View/Set | Resource 与 View/Heap | 引用不复制内容，不自动管理使用期 |
| 验收维度 | VUID + 数值/图像 | InfoQueue + 数值/图像 | API 合法与业务正确分别证明 |

先为每项指出源码中的创建/绑定位置，以及证明它成立的实际日志、字节或图像。数值中的设备限制以查询为准；固定实验尺寸不当作所有设备的上限。

## nvpro_core2 框架对照

把 nvvk::Context 对应到 instance/device/queue，把 nvapp::Application 对应到平台与帧循环；自定义 SceneVk 仍负责实际绘制。

这部分是原生 Vulkan 各课的框架对照，框架可运行入口仍为原有六课。如果本节需要新接入工作，会按正文说明；不把原生实验当作已完成的框架实现。

## 运行与实际检查

| 路线 | 实际实验 |
| --- | --- |
| Vulkan 原生 | [vk11_capabilities](../../lessons/vk11_capabilities/README.md) |
| DirectX 12 | [dx11_capabilities](../../../../DirectX12/lessons/dx11_capabilities/README.md) |

从合集根目录执行；普通图形程序 Esc 退出，设备/计算/诊断程序输出结果后结束。从“开始学习.cmd”进入本页时，表格可直接点击运行。

```powershell
cmake --preset vulkan-native
cmake --build --preset vulkan-native-debug --target vk11_capabilities --parallel 4
.\Vulkan\native\build\bin\Debug\vk11_capabilities.exe
ctest --preset vulkan-native-debug -R "^vk11_capabilities$" --output-on-failure
```

检查三个层次：创建/绑定/提交的接口约束；数据读回或目标画面；资源使用结束后的回收。诊断实验用合法信息或查询证明渠道可用，不故意保留 API 错误。图形 smoke 的像素检查与数值实验的逐项检查不同，具体实现见源码。

## 关键实现：从当前源码定位

来自 [Vulkan/native/common/advanced_labs.cpp](../../common/advanced_labs.cpp#L146)，第 146—189 行。下列是当前仓库的局部切片，可能在函数中间结束；完整上下文请打开文件，切片不作为单独可编译程序。

```cpp
void capabilities(const VkContextRef& c) {
    VkPhysicalDeviceProperties p{};
    vkGetPhysicalDeviceProperties(c.physical, &p);
    VkPhysicalDeviceMemoryProperties m{};
    vkGetPhysicalDeviceMemoryProperties(c.physical, &m);
    std::cout << "maxPushConstantsSize=" << p.limits.maxPushConstantsSize
              << ", minUniformBufferOffsetAlignment=" << p.limits.minUniformBufferOffsetAlignment
              << ", maxComputeWorkGroupInvocations=" << p.limits.maxComputeWorkGroupInvocations
              << '\n';
    for (uint32_t i = 0; i < m.memoryTypeCount; ++i)
        std::cout << "memoryType[" << i << "] heap=" << m.memoryTypes[i].heapIndex
                  << " properties=" << m.memoryTypes[i].propertyFlags << '\n';
    require(p.limits.maxPushConstantsSize >= 80, "80-byte Draw exceeds device limit");
    VkFormatProperties depth{};
    vkGetPhysicalDeviceFormatProperties(c.physical, VK_FORMAT_D32_SFLOAT, &depth);
    require((depth.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) != 0,
            "D32 unsupported");
    std::cout << "Verified required limits and depth format\n";
}
// upload → device-local → readback 完整闭环；每次拷贝后的消费者访问不同。
void stagingCopy(const VkContextRef& c) {
    constexpr size_t count = 257;
    std::vector<uint32_t> original(count);
    for (uint32_t i = 0; i < count; ++i)
        original[i] = i * 17 + 9;
    VkBufferResource upload, gpu, readback;
    upload.init(c, count * 4, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
    upload.upload(original.data(), count * 4);
    gpu.init(c,
             count * 4,
             VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
             VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    readback.init(c, count * 4, VK_BUFFER_USAGE_TRANSFER_DST_BIT);
    immediate(c, [&](VkCommandBuffer cmd) {
        VkBufferCopy copy{0, 0, count * 4};
        vkCmdCopyBuffer(cmd, upload.buffer, gpu.buffer, 1, &copy);
        bufferDependency(cmd,
                         gpu.buffer,
                         VK_PIPELINE_STAGE_2_COPY_BIT,
                         VK_ACCESS_2_TRANSFER_WRITE_BIT,
                         VK_PIPELINE_STAGE_2_COPY_BIT,
                         VK_ACCESS_2_TRANSFER_READ_BIT);
        vkCmdCopyBuffer(cmd, gpu.buffer, readback.buffer, 1, &copy);
        bufferDependency(cmd,
```

阅读时按“前置对象 → 参数/布局 → 录制/调用 → 完成/可见性 → 释放”五步标注。共享 helper 减少重复代码，但不会取消每课的 API 契约。

## 受控修改实验

画出一个三角形所需的五类对象，并说明哪些对象保存字节、哪些只保存解释或引用。

先记录预测，再只改一个因素；保留基线与修改后的输出。不要同时改变 shader、资源范围和同步，这样即使结果变好也无法归因。

### 参考推导与通过标准

Buffer/Image 保存内容，ImageView/descriptor/布局解释或引用内容，pipeline 约定执行方式；Queue 执行已录制工作。引用不会复制资源，也不会延长 GPU 资源的合法使用期。

## 常见失败与排查顺序

把所有初始化都理解成“创建窗口”，会使设备失败、目标失败和管线失败难以区分。先标注责任再看函数名。

先看第一条错误和当前执行路径，再看实际数据/绑定，最后看 shader 与画面。需要 GPU 证据时抓取正常交互帧；仅四帧的 smoke 适合自动检查，不适合手动抓帧。

## 自测与本课交付

能否不看源码回答：画出一个三角形所需的五类对象，并说明哪些对象保存字节、哪些只保存解释或引用。

提交一份简短记录：输入与假设、唯一修改、预期、实际证据、原因、是否恢复基线。如果只能复述函数名而说不清何时允许访问/释放，先回到本课的数据流。

## 官方阅读位置与当前工程差异

[指定 Vulkan 教程对应位置](https://docs.vulkan.net.cn/tutorial/latest/01_Overview.html)。DX12 路线按同一概念顺序建立映射，其对象与约束以 Microsoft 文档和本后端代码为准。49—54 课包含在基础/compute 之后加入的工程扩展。

[版本与资料说明](../../../../docs/REFERENCES.md) · [调试](../../../../docs/DEBUGGING.md) · [功能完成度](../../../../docs/FEATURE_MATRIX.md)
