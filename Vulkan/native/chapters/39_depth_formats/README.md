# 第 39 课 · 深度格式、用途与样本支持 · Vulkan 原生

[54 课目录](../../COURSE.md) · [对照精讲](../../../../docs/compare/07_targets_depth_msaa.md) · [上一课](../38_color_space/README.md) · [下一课](../40_depth_test/README.md)

## 本课目标与先修

学习目标：解释“深度格式、用途与样本支持”解决的问题，找到实际对象和参数，运行对应实验并用数据/画面证明。先完成前一课，并能说明它交付的对象与完成条件；本课在该依赖上增加新契约。

## 为什么需要这一层

深度数据与颜色一样需要正确格式、使用方式和支持查询。选择 D32 并不是保证所有其他深度/模板方案都可用；加入模板或 MSAA 后要重新检查相关条件。

## Vulkan 原生 的对象、参数与执行推导

查询 D32、D24S8、D16 的 optimal tiling features，检查 DEPTH_STENCIL_ATTACHMENT。view 的 aspect 要与实际使用一致；若包含 stencil，不能无条件只按纯 depth 格式处理所有布局与 view。

## 对照 DirectX 12：相同问题的另一种分工

FORMAT_SUPPORT 的 depth-stencil 支持与 MULTISAMPLE_QUALITY_LEVELS 分别查询。DSV 的格式/维度和 PSO 的 DSVFormat、sample count 要一致，resource flags 还需允许深度用途。

## 关键参数与可检查不变量

| 检查项 | Vulkan 表达 | DX12 表达 | 不变量 / 证据 |
| --- | --- | --- | --- |
| 深度支持 | format features 查询 | FORMAT_SUPPORT 查询 | 用途成立后才选择格式 |
| 模板条件 | aspect 与格式一致 | DSV 格式与资源一致 | 纯 depth 与 depth/stencil 不混同 |
| 多样本交集 | 颜色/深度都支持 | 两种格式 quality>0 | pipeline/PSO 附件同时匹配 |

先为每项指出源码中的创建/绑定位置，以及证明它成立的实际日志、字节或图像。数值中的设备限制以查询为准；固定实验尺寸不当作所有设备的上限。

## nvpro_core2 框架对照

RenderTarget::CreateInfo 中 depthFormat 也是明确需求。框架替你分配附件，不代表任意格式和 sample count 组合都成立。

这部分是原生 Vulkan 各课的框架对照，框架可运行入口仍为原有六课。如果本节需要新接入工作，会按正文说明；不把原生实验当作已完成的框架实现。

## 运行与实际检查

| 路线 | 实际实验 |
| --- | --- |
| Vulkan 原生 | [vk46_depth_formats](../../lessons/vk46_depth_formats/README.md) |
| DirectX 12 | [dx46_depth_formats](../../../../DirectX12/lessons/dx46_depth_formats/README.md) |

从合集根目录执行；普通图形程序 Esc 退出，设备/计算/诊断程序输出结果后结束。从“开始学习.cmd”进入本页时，表格可直接点击运行。

```powershell
cmake --preset vulkan-native
cmake --build --preset vulkan-native-debug --target vk46_depth_formats --parallel 4
.\Vulkan\native\build\bin\Debug\vk46_depth_formats.exe
ctest --preset vulkan-native-debug -R "^vk46_depth_formats$" --output-on-failure
```

检查三个层次：创建/绑定/提交的接口约束；数据读回或目标画面；资源使用结束后的回收。诊断实验用合法信息或查询证明渠道可用，不故意保留 API 错误。图形 smoke 的像素检查与数值实验的逐项检查不同，具体实现见源码。

## 关键实现：从当前源码定位

来自 [Vulkan/native/common/advanced_labs.cpp](../../common/advanced_labs.cpp#L874)，第 874—917 行。下列是当前仓库的局部切片，可能在函数中间结束；完整上下文请打开文件，切片不作为单独可编译程序。

```cpp
        for (uint32_t i = 0; i < 128; ++i)
            require(changed[i] == i * 5 + 11, "Updated descriptor points at wrong storage");
        std::cout << "Verified descriptor replacement after prior submission completion\n";
    } else if (id == 46) {
        for (auto format :
             {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D16_UNORM}) {
            VkFormatProperties p{};
            vkGetPhysicalDeviceFormatProperties(c.physical, format, &p);
            std::cout << "depth format=" << format
                      << ", optimal features=" << p.optimalTilingFeatures << '\n';
        }
    } else if (id == 47) {
        VkBufferResource buffer;
        buffer.init(c, 257, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
        VkMemoryRequirements r{};
        vkGetBufferMemoryRequirements(c.device, buffer.buffer, &r);
        require(r.size >= 257 && r.alignment > 0, "Invalid memory requirements");
        std::cout << "requested=257, allocation=" << r.size << ", alignment=" << r.alignment
                  << ", memoryTypeBits=" << r.memoryTypeBits << '\n';
    } else if (id == 48) {
        VkFenceCreateInfo fi{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
        fi.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        VkFence fence{};
        vkCheck(vkCreateFence(c.device, &fi, nullptr, &fence));
        require(vkGetFenceStatus(c.device, fence) == VK_SUCCESS, "Initial fence not signaled");
        vkCheck(vkResetFences(c.device, 1, &fence));
        require(vkGetFenceStatus(c.device, fence) == VK_NOT_READY,
                "Reset fence signaled unexpectedly");
        VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};
        vkCheck(vkQueueSubmit(c.queue, 1, &submit, fence));
        vkCheck(vkWaitForFences(c.device, 1, &fence, VK_TRUE, UINT64_MAX));
        vkDestroyFence(c.device, fence, nullptr);
        std::cout << "Verified signaled -> reset -> submitted -> complete fence lifecycle\n";
    } else if (id == 51) {
        static_assert(sizeof(Frame) == 64 && sizeof(Draw) == 80 && offsetof(Draw, settings) == 64);
        ComputeKernel kernel(c);
        kernel.execute(1, 128, 0, 0, 2);
        auto r = readWords(kernel.output, 128);
        for (uint32_t i = 0; i < 128; ++i)
            require(r[i] == i * 7 + 13, "UBO layout mismatch");
        std::cout
            << "Verified Frame=64, Draw=80, settings offset=64 and GPU uniform vector decoding\n";
    } else if (id == 54) {
        capabilities(c);
```

阅读时按“前置对象 → 参数/布局 → 录制/调用 → 完成/可见性 → 释放”五步标注。共享 helper 减少重复代码，但不会取消每课的 API 契约。

## 受控修改实验

为颜色和深度选择共同支持的样本数，写出如何取合法交集。

先记录预测，再只改一个因素；保留基线与修改后的输出。不要同时改变 shader、资源范围和同步，这样即使结果变好也无法归因。

### 参考推导与通过标准

Vulkan 检查颜色/深度 sample flags 的交集并结合实际格式；DX12 分别查询对应格式的指定样本数，只有两者支持才能组合。

## 常见失败与排查顺序

颜色支持 4x、深度仅单样本，却仍创建一个 MSAA pipeline，是目标一致性问题而不是光照公式问题。

先看第一条错误和当前执行路径，再看实际数据/绑定，最后看 shader 与画面。需要 GPU 证据时抓取正常交互帧；仅四帧的 smoke 适合自动检查，不适合手动抓帧。

## 自测与本课交付

能否不看源码回答：为颜色和深度选择共同支持的样本数，写出如何取合法交集。

提交一份简短记录：输入与假设、唯一修改、预期、实际证据、原因、是否恢复基线。如果只能复述函数名而说不清何时允许访问/释放，先回到本课的数据流。

## 官方阅读位置与当前工程差异

[指定 Vulkan 教程对应位置](https://docs.vulkan.net.cn/tutorial/latest/07_Depth_buffering.html)。DX12 路线按同一概念顺序建立映射，其对象与约束以 Microsoft 文档和本后端代码为准。49—54 课包含在基础/compute 之后加入的工程扩展。

[版本与资料说明](../../../../docs/REFERENCES.md) · [调试](../../../../docs/DEBUGGING.md) · [功能完成度](../../../../docs/FEATURE_MATRIX.md)
