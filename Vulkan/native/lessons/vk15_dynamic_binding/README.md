# vk15_dynamic_binding · 动态常量范围

[54 课目录](../../COURSE.md)

## 本课独立工程与关键注释

[main.cpp](main.cpp) · [本地辅助实现](support/) · [本课着色器](shaders/) · [CMakeLists.txt](CMakeLists.txt)

本目录包含独立课程源码，关键位置有中文注释。单独复制整个文件夹后可用 `cmake -S . -B build -A x64` 配置、`cmake --build build --config Debug` 构建；不需要其他章节源码。

## 目的与实际执行

使用 UNIFORM_BUFFER_DYNAMIC，bind 时传一个动态偏移。三个槽使用设备要求的 stride，descriptor range 保持 16 字节；shader 读取每槽不同 factor/bias，逐项验证结果。

本程序的 C++、辅助实现、着色器和构建脚本均位于本目录，关键步骤含中文注释。设备/API 检查、数值 compute 与图形渲染采用不同证据；不是每个程序都会弹出窗口。

## 构建和运行

```powershell
cmake --preset vulkan-native
cmake --build --preset vulkan-native-debug --target vk15_dynamic_binding --parallel 4
.\Vulkan\native\build\bin\Debug\vk15_dynamic_binding.exe
```

## 深入学习：数据、接口与执行

- [第 31 课：动态 UBO 与 Root CBV 地址选择](../../chapters/31_dynamic_offsets/README.md)

### 本课的真实源码切片

来自 [Vulkan/native/lessons/vk15_dynamic_binding/main.cpp](main.cpp#L157)，第 157—199 行；这是现有实现的局部摘录，末尾可能处于函数中间，不是可单独编译的完整程序。

```cpp
int runChapter(int argc, char** argv) {
    auto options = labOptions(argc, argv, 15);
    nativeShaders = options.directory / "shaders";
    validationErrors = 0;
    {
        NativeDevice owner(nullptr, false);
        const auto& c = owner.c;
        {
            ComputeKernel kernel(c);
            uint32_t mode = ((1));
            uint32_t count = (128);
            for (uint32_t slot = 0; slot < ((3u)); ++slot) {
                uint32_t factor = (3), bias = (7);
                kernel.execute(mode, count, factor, bias, slot);
                auto result = readWords(kernel.output, mode == 2 ? (count + 63) / 64 : count);
                if (mode == 2) {
                    uint32_t total = std::accumulate(result.begin(), result.end(), 0u);
                    require(total == count * (count + 1) / 2, "Shared-memory reduction mismatch");
                } else {
                    if (mode == 1) {
                        factor = slot == 0 ? 2 : slot == 1 ? 5 : 7;
                        bias = slot == 0 ? 3 : slot == 1 ? 11 : 13;
                    }
                    for (uint32_t i = 0; i < count; ++i)
                        require(result[i] == i * factor + bias,
                                "Compute/dynamic-binding result mismatch");
                }
            }
            std::cout << "Verified experiment " << 15 << ", items=" << count
                      << ", uniform stride=" << kernel.stride << '\n';
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
| Vulkan 原生 | [vk15_dynamic_binding](README.md) |
| 对照后端 | [dx15_dynamic_binding](../../../../DirectX12/lessons/dx15_dynamic_binding/README.md) |

### 先预测，再验证

把三个槽的参数互换而不改变 shader，预测结果应如何变化。

参考推导：输出应随绑定范围改变。若 CPU 内容变了而输出仍来自旧槽，检查实际 bind offset/address，而不是重新编译 shader。

## 验证与边界

运行 `--smoke` 执行有限自动检查。数值实验读取实际 GPU 结果，图形实验检查非空图像并可查看读回。交互重建与性能还需额外证据；窗口 renderer 当前保留串行等待，双槽计算实验不会被描述为完整多帧呈现引擎。
