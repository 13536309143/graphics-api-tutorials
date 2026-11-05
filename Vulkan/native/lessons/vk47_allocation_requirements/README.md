# vk47_allocation_requirements · 分配大小与对齐

[54 课目录](../../COURSE.md)

## 本课独立工程与关键注释

[main.cpp](main.cpp) · [本地辅助实现](support/) · [本课着色器](shaders/) · [CMakeLists.txt](CMakeLists.txt)

本目录包含独立课程源码，关键位置有中文注释。单独复制整个文件夹后可用 `cmake -S . -B build -A x64` 配置、`cmake --build build --config Debug` 构建；不需要其他章节源码。

## 目的与实际执行

memoryTypeBits 是资源允许的类型集合，propertyFlags 是内存属性。实验为 257 字节缓冲查询实际 size/alignment/type bits，再在允许集合中选择所需属性。non-coherent 的 flush/invalidate 还需符合原子尺寸对齐。

本程序的 C++、辅助实现、着色器和构建脚本均位于本目录，关键步骤含中文注释。设备/API 检查、数值 compute 与图形渲染采用不同证据；不是每个程序都会弹出窗口。

## 构建和运行

```powershell
cmake --preset vulkan-native
cmake --build --preset vulkan-native-debug --target vk47_allocation_requirements --parallel 4
.\Vulkan\native\build\bin\Debug\vk47_allocation_requirements.exe
```

## 深入学习：数据、接口与执行

- [第 25 课：内存类型、Heap 与分配要求](../../chapters/25_memory/README.md)

### 本课的真实源码切片

来自 [Vulkan/native/lessons/vk47_allocation_requirements/main.cpp](main.cpp#L23)，第 23—44 行；这是现有实现的局部摘录，末尾可能处于函数中间，不是可单独编译的完整程序。

```cpp
int runChapter(int argc, char** argv) {
    auto options = labOptions(argc, argv, 47);
    nativeShaders = options.directory / "shaders";
    validationErrors = 0;
    {
        NativeDevice owner(nullptr, false);
        const auto& c = owner.c;
        {
            inspection(c);
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
| Vulkan 原生 | [vk47_allocation_requirements](README.md) |
| 对照后端 | [dx47_allocation_requirements](../../../../DirectX12/lessons/dx47_allocation_requirements/README.md) |

### 先预测，再验证

比较请求 257 字节、返回的 allocation size、常量槽 stride 三者，解释不同来源。

参考推导：257 是应用内容大小，allocation size 来自内存分配约束，常量 stride 来自接口偏移对齐。三者可以不同而同时正确。

## 验证与边界

运行 `--smoke` 执行有限自动检查。数值实验读取实际 GPU 结果，图形实验检查非空图像并可查看读回。交互重建与性能还需额外证据；窗口 renderer 当前保留串行等待，双槽计算实验不会被描述为完整多帧呈现引擎。
