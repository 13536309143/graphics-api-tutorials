# vk33_debug_labels · GPU 调试标签

[54 课目录](../../COURSE.md)

## 目的与实际执行

枚举 layer 后创建 debug messenger，并按 severity/type 分类。实验额外注册 INFO 回调，提交应用信息并确认回调收到。外部 loader manifest 诊断与应用 VUID 分别记录。GPU 区域标签通过 debug-utils entry points 录制。

本程序有独立实验选择与验证路径，公共实现集中在 advanced_labs.cpp。设备/API 检查、数值 compute 与图形渲染采用不同证据；不是每个程序都会弹出窗口。

## 构建和运行

```powershell
cmake --preset vulkan-native
cmake --build --preset vulkan-native-debug --target vk33_debug_labels --parallel 4
.\Vulkan\native\build\bin\Debug\vk33_debug_labels.exe
```

## 深入学习：数据、接口与执行

- [第 05 课：验证层、调试队列与诊断证据](../../chapters/05_validation/README.md)

### 本课的真实源码切片

来自 [Vulkan/native/common/advanced_labs.cpp](../../common/advanced_labs.cpp#L978)，第 978—1021 行；这是现有实现的局部摘录，末尾可能处于函数中间，不是可单独编译的完整程序。

```cpp
            auto begin = reinterpret_cast<PFN_vkCmdBeginDebugUtilsLabelEXT>(
                vkGetDeviceProcAddr(c.device, "vkCmdBeginDebugUtilsLabelEXT"));
            auto end = reinterpret_cast<PFN_vkCmdEndDebugUtilsLabelEXT>(
                vkGetDeviceProcAddr(c.device, "vkCmdEndDebugUtilsLabelEXT"));
            require(begin && end, "Debug label entry points missing");
            immediate(c, [&](VkCommandBuffer cmd) {
                VkDebugUtilsLabelEXT label{VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT};
                label.pLabelName = "Course/Upload pass";
                label.color[1] = 1;
                begin(cmd, &label);
                end(cmd);
            });
            std::cout << "Submitted named GPU debug region\n";
        } else if (id == 32) {
            VkPipelineCacheCreateInfo info{VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO};
            VkPipelineCache first{}, second{};
            vkCheck(vkCreatePipelineCache(c.device, &info, nullptr, &first));
            {
                ComputeKernel k(c, first);
                k.execute(0, 257, 3, 7);
                auto r = readWords(k.output, 257);
                require(r[256] == 775, "Cached compute output mismatch");
            }
            size_t bytes{};
            vkCheck(vkGetPipelineCacheData(c.device, first, &bytes, nullptr));
            std::vector<uint8_t> data(bytes);
            vkCheck(vkGetPipelineCacheData(c.device, first, &bytes, data.data()));
            info.initialDataSize = bytes;
            info.pInitialData = data.data();
            vkCheck(vkCreatePipelineCache(c.device, &info, nullptr, &second));
            {
                ComputeKernel k(c, second);
                k.execute(0, 257, 3, 7);
                require(readWords(k.output, 257)[256] == 775, "Reloaded pipeline cache mismatch");
            }
            vkDestroyPipelineCache(c.device, second, nullptr);
            vkDestroyPipelineCache(c.device, first, nullptr);
            std::cout << "Verified pipeline-cache serialization and reload, bytes=" << bytes
                      << '\n';
        } else {
            ComputeKernel kernel(c);
            uint32_t mode = id == 30 ? 2 : id == 14 || id == 15 ? 1 : 0;
            uint32_t count = id == 28 || id == 30 ? 257 : 128;
            for (uint32_t slot = 0; slot < (id == 15 ? 3u : 1u); ++slot) {
```

### 同主题跨 API 导航

| 路线 | 程序 |
| --- | --- |
| Vulkan 原生 | [vk33_debug_labels](README.md) |
| 对照后端 | [dx33_debug_labels](../../../../DirectX12/lessons/dx33_debug_labels/README.md) |

### 先预测，再验证

注入一条 INFO 应用消息，确认它会进入日志/回调而不会增加 API 错误计数。

参考推导：消息严重度与错误计数策略独立。合法的应用信息用于测试诊断渠道，不应冒充 validation error 或通过清空错误队列让检查成功。

## 验证与边界

运行 `--smoke` 执行有限自动检查。数值实验读取实际 GPU 结果，图形实验检查非空图像并可查看读回。交互重建与性能还需额外证据；窗口 renderer 当前保留串行等待，双槽计算实验不会被描述为完整多帧呈现引擎。
