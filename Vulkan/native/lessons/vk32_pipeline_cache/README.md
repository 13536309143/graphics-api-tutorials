# vk32_pipeline_cache · 管线缓存与重建

[54 课目录](../../COURSE.md)

## 目的与实际执行

timestampValidBits 与 timestampPeriod 决定计时解释；本例记录填充操作前后时间并处理有效位。pipeline cache 在同一设备/驱动条件下保存并重建 compute pipeline，实际执行后再核对数值。

本程序有独立实验选择与验证路径，公共实现集中在 advanced_labs.cpp。设备/API 检查、数值 compute 与图形渲染采用不同证据；不是每个程序都会弹出窗口。

## 构建和运行

```powershell
cmake --preset vulkan-native
cmake --build --preset vulkan-native-debug --target vk32_pipeline_cache --parallel 4
.\Vulkan\native\build\bin\Debug\vk32_pipeline_cache.exe
```

## 深入学习：数据、接口与执行

- [第 53 课：GPU 时间戳、管线缓存与测量边界](../../chapters/53_measure_cache/README.md)

### 本课的真实源码切片

来自 [Vulkan/native/common/advanced_labs.cpp](../../common/advanced_labs.cpp#L984)，第 984—1027 行；这是现有实现的局部摘录，末尾可能处于函数中间，不是可单独编译的完整程序。

```cpp
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
                uint32_t factor = id == 16 ? 9 : 3, bias = id == 16 ? 17 : 7;
                kernel.execute(mode, count, factor, bias, slot);
                auto result = readWords(kernel.output, mode == 2 ? (count + 63) / 64 : count);
                if (mode == 2) {
                    uint32_t total = std::accumulate(result.begin(), result.end(), 0u);
                    require(total == count * (count + 1) / 2, "Shared-memory reduction mismatch");
```

### 同主题跨 API 导航

| 路线 | 程序 |
| --- | --- |
| Vulkan 原生 | [vk32_pipeline_cache](README.md) |
| 对照后端 | [dx32_pipeline_cache](../../../../DirectX12/lessons/dx32_pipeline_cache/README.md) |

### 先预测，再验证

列出一个公平跨 API 性能实验至少需要固定的六项条件。

参考推导：工作量、分辨率、shader/材质、场景/相机、采样/呈现策略、构建配置；还应预热、重复采样并报告 CPU/GPU/整帧时间及波动。

## 验证与边界

运行 `--smoke` 执行有限自动检查。数值实验读取实际 GPU 结果，图形实验检查非空图像并可查看读回。交互重建与性能还需额外证据；窗口 renderer 当前保留串行等待，双槽计算实验不会被描述为完整多帧呈现引擎。
