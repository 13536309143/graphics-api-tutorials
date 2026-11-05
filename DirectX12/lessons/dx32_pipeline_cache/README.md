# dx32_pipeline_cache · 管线缓存与重建

[54 课目录](../../COURSE.md)

## 本课独立工程与关键注释

[main.cpp](main.cpp) · [本地辅助实现](support/) · [本课着色器](shaders/) · [CMakeLists.txt](CMakeLists.txt)

本目录包含独立课程源码，关键位置有中文注释。单独复制整个文件夹后可用 `cmake -S . -B build -A x64` 配置、`cmake --build build --config Debug` 构建；不需要其他章节源码。

## 目的与实际执行

queue timestamp frequency 将 query ticks 转成时间，结果经 ResolveQueryData/readback 等待后读取。缓存 PSO 重建复用完全相同的编译字节码；Debug 重编译可能引入变化，不能仅凭源码相同断言 cache 一定兼容。

本程序的 C++、辅助实现、着色器和构建脚本均位于本目录，关键步骤含中文注释。设备/API 检查、数值 compute 与图形渲染采用不同证据；不是每个程序都会弹出窗口。

## 构建和运行

```powershell
cmake --preset dx12
cmake --build --preset dx12-debug --target dx32_pipeline_cache --parallel 4
.\DirectX12\build\bin\Debug\dx32_pipeline_cache.exe
```

## 深入学习：数据、接口与执行

- [第 53 课：GPU 时间戳、管线缓存与测量边界](../../chapters/53_measure_cache/README.md)

### 本课的真实源码切片

来自 [DirectX12/lessons/dx32_pipeline_cache/main.cpp](main.cpp#L136)，第 136—167 行；这是现有实现的局部摘录，末尾可能处于函数中间，不是可单独编译的完整程序。

```cpp
int runChapter(int argc, char** argv) {
    auto options = labOptions(argc, argv, 32);
    DxDevice c(options.warp);
    {
        ComPtr<ID3DBlob> cache, exactShader;
        {
            ComputeKernel first(c, options);
            first.execute(0, 257, 3, 7);
            require(readBuffer(first.readback.Get(), 257)[256] == 775,
                    "Initial cached PSO result mismatch");
            dxCheck(first.pipeline->GetCachedBlob(&cache));
            exactShader = first.compiledShader;
        }
        {
            ComputeKernel second(c, options, cache.Get(), exactShader.Get());
            second.execute(0, 257, 3, 7);
            require(readBuffer(second.readback.Get(), 257)[256] == 775,
                    "Reloaded cached PSO mismatch");
        }
        std::cout << "Verified cached PSO reload, bytes=" << cache->GetBufferSize() << '\n';
    }
    c.checkMessages();
    return 0;
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
| DirectX 12 | [dx32_pipeline_cache](README.md) |
| 对照后端 | [vk32_pipeline_cache](../../../Vulkan/native/lessons/vk32_pipeline_cache/README.md) |

### 先预测，再验证

列出一个公平跨 API 性能实验至少需要固定的六项条件。

参考推导：工作量、分辨率、shader/材质、场景/相机、采样/呈现策略、构建配置；还应预热、重复采样并报告 CPU/GPU/整帧时间及波动。

## 验证与边界

运行 `--smoke` 执行有限自动检查。数值实验读取实际 GPU 结果，图形实验检查非空图像并可查看读回。交互重建与性能还需额外证据；窗口 renderer 当前保留串行等待，双槽计算实验不会被描述为完整多帧呈现引擎。
