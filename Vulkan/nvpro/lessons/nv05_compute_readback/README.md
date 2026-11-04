# nv05_compute_readback · Compute 与读回验证

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

来自 [Vulkan/common/scene_vk.hpp](../../../common/scene_vk.hpp#L442)，第 442—471 行；这是现有实现的局部摘录，末尾可能处于函数中间，不是可单独编译的完整程序。

```cpp
inline void computeSquares(const VkContextRef& c, const std::filesystem::path& shaders) {
    VkBufferResource storage;
    storage.init(c, 256 * 4, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
    VkDescriptorSetLayoutBinding binding{
        0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr};
    VkDescriptorSetLayoutCreateInfo li{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    li.bindingCount = 1;
    li.pBindings = &binding;
    VkDescriptorSetLayout layout;
    vkCheck(vkCreateDescriptorSetLayout(c.device, &li, nullptr, &layout));
    VkPipelineLayoutCreateInfo pl{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    pl.setLayoutCount = 1;
    pl.pSetLayouts = &layout;
    VkPipelineLayout pipelineLayout;
    vkCheck(vkCreatePipelineLayout(c.device, &pl, nullptr, &pipelineLayout));
    VkDescriptorPoolSize size{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1};
    VkDescriptorPoolCreateInfo pool{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    pool.maxSets = 1;
    pool.poolSizeCount = 1;
    pool.pPoolSizes = &size;
    VkDescriptorPool descriptorPool;
    vkCheck(vkCreateDescriptorPool(c.device, &pool, nullptr, &descriptorPool));
    VkDescriptorSetAllocateInfo ai{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    ai.descriptorPool = descriptorPool;
    ai.descriptorSetCount = 1;
    ai.pSetLayouts = &layout;
    VkDescriptorSet set;
    vkCheck(vkAllocateDescriptorSets(c.device, &ai, &set));
    VkDescriptorBufferInfo bi{storage.buffer, 0, 256 * 4};
    VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
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
