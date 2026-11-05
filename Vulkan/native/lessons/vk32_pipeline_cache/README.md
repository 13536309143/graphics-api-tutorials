# vk32_pipeline_cache · 管线缓存与重建

[54 课目录](../../COURSE.md)

## 本课独立工程与关键注释

[main.cpp](main.cpp) · [本地辅助实现](support/) · [本课着色器](shaders/) · [CMakeLists.txt](CMakeLists.txt)

本目录包含独立课程源码，关键位置有中文注释。单独复制整个文件夹后可用 `cmake -S . -B build -A x64` 配置、`cmake --build build --config Debug` 构建；不需要其他章节源码。

## 目的与实际执行

timestampValidBits 与 timestampPeriod 决定计时解释；本例记录填充操作前后时间并处理有效位。pipeline cache 在同一设备/驱动条件下保存并重建 compute pipeline，实际执行后再核对数值。

本程序的 C++、辅助实现、着色器和构建脚本均位于本目录，关键步骤含中文注释。设备/API 检查、数值 compute 与图形渲染采用不同证据；不是每个程序都会弹出窗口。

## 构建和运行

```powershell
cmake --preset vulkan-native
cmake --build --preset vulkan-native-debug --target vk32_pipeline_cache --parallel 4
.\Vulkan\native\build\bin\Debug\vk32_pipeline_cache.exe
```

## 深入学习：数据、接口与执行

- [第 53 课：GPU 时间戳、管线缓存与测量边界](../../chapters/53_measure_cache/README.md)

### 本课的真实源码切片

来自 [Vulkan/native/lessons/vk32_pipeline_cache/main.cpp](main.cpp#L157)，第 157—202 行；这是现有实现的局部摘录，末尾可能处于函数中间，不是可单独编译的完整程序。

```cpp
int runChapter(int argc, char** argv) {
    auto options = labOptions(argc, argv, 32);
    nativeShaders = options.directory / "shaders";
    validationErrors = 0;
    {
        NativeDevice owner(nullptr, false);
        const auto& c = owner.c;
        {
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
        }
    }
    std::cout << "Validation errors: " << validationErrors << '\n';
    return validationErrors ? 1 : 0;
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

| 路线 | 程序 |
| --- | --- |
| Vulkan 原生 | [vk32_pipeline_cache](README.md) |
| 对照后端 | [dx32_pipeline_cache](../../../../DirectX12/lessons/dx32_pipeline_cache/README.md) |

### 先预测，再验证

列出一个公平跨 API 性能实验至少需要固定的六项条件。

参考推导：工作量、分辨率、shader/材质、场景/相机、采样/呈现策略、构建配置；还应预热、重复采样并报告 CPU/GPU/整帧时间及波动。

## 验证与边界

运行 `--smoke` 执行有限自动检查。数值实验读取实际 GPU 结果，图形实验检查非空图像并可查看读回。交互重建与性能还需额外证据；窗口 renderer 当前保留串行等待，双槽计算实验不会被描述为完整多帧呈现引擎。
