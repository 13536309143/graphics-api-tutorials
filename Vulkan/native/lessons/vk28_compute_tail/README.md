# vk28_compute_tail · 非整组尾部计算

[54 课目录](../../COURSE.md)

## 本课独立工程与关键注释

[main.cpp](main.cpp) · [本地辅助实现](support/) · [本课着色器](shaders/) · [CMakeLists.txt](CMakeLists.txt)

本目录包含独立课程源码，关键位置有中文注释。单独复制整个文件夹后可用 `cmake -S . -B build -A x64` 配置、`cmake --build build --config Debug` 构建；不需要其他章节源码。

## 目的与实际执行

storage buffer range 覆盖足够字节，shader 以 i<count 防止尾部写越界。host 用向上取整组数并逐项核对 affine 公式。buffer 的 usage、descriptor 类型与 shader 声明必须相容。

本程序的 C++、辅助实现、着色器和构建脚本均位于本目录，关键步骤含中文注释。设备/API 检查、数值 compute 与图形渲染采用不同证据；不是每个程序都会弹出窗口。

## 构建和运行

```powershell
cmake --preset vulkan-native
cmake --build --preset vulkan-native-debug --target vk28_compute_tail --parallel 4
.\Vulkan\native\build\bin\Debug\vk28_compute_tail.exe
```

## 深入学习：数据、接口与执行

- [第 47 课：Storage/UAV、边界与 257 项实验](../../chapters/47_storage_tail/README.md)

### 本课的真实源码切片

来自 [Vulkan/native/lessons/vk28_compute_tail/main.cpp](main.cpp#L157)，第 157—199 行；这是现有实现的局部摘录，末尾可能处于函数中间，不是可单独编译的完整程序。

```cpp
int runChapter(int argc, char** argv) {
    auto options = labOptions(argc, argv, 28);
    nativeShaders = options.directory / "shaders";
    validationErrors = 0;
    {
        NativeDevice owner(nullptr, false);
        const auto& c = owner.c;
        {
            ComputeKernel kernel(c);
            uint32_t mode = ((0));
            uint32_t count = (257);
            for (uint32_t slot = 0; slot < ((1u)); ++slot) {
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
            std::cout << "Verified experiment " << 28 << ", items=" << count
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
| Vulkan 原生 | [vk28_compute_tail](README.md) |
| 对照后端 | [dx28_compute_tail](../../../../DirectX12/lessons/dx28_compute_tail/README.md) |

### 先预测，再验证

只把 CPU 验证改成 257 而仍分配 256 项，会产生什么问题？

参考推导：可能读取越界；只扩容量却不扩组数则尾项未写。正确验收不能用忽略最后一项来让测试通过。

## 验证与边界

运行 `--smoke` 执行有限自动检查。数值实验读取实际 GPU 结果，图形实验检查非空图像并可查看读回。交互重建与性能还需额外证据；窗口 renderer 当前保留串行等待，双槽计算实验不会被描述为完整多帧呈现引擎。
