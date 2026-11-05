# vk39_queue_properties · 队列能力与属性

[54 课目录](../../COURSE.md)

## 本课独立工程与关键注释

[main.cpp](main.cpp) · [本地辅助实现](support/) · [本课着色器](shaders/) · [CMakeLists.txt](CMakeLists.txt)

本目录包含独立课程源码，关键位置有中文注释。单独复制整个文件夹后可用 `cmake -S . -B build -A x64` 配置、`cmake --build build --config Debug` 构建；不需要其他章节源码。

## 目的与实际执行

queue family 具有 flags、queueCount 和 timestampValidBits。graphics/compute family 隐含支持传输，但 present support 需要对实际 surface 查询。跨 family 使用 EXCLUSIVE 资源还涉及所有权转移，不能只添加 semaphore。

本程序的 C++、辅助实现、着色器和构建脚本均位于本目录，关键步骤含中文注释。设备/API 检查、数值 compute 与图形渲染采用不同证据；不是每个程序都会弹出窗口。

## 构建和运行

```powershell
cmake --preset vulkan-native
cmake --build --preset vulkan-native-debug --target vk39_queue_properties --parallel 4
.\Vulkan\native\build\bin\Debug\vk39_queue_properties.exe
```

## 深入学习：数据、接口与执行

- [第 07 课：队列族、命令队列与并行边界](../../chapters/07_queues/README.md)

### 本课的真实源码切片

来自 [Vulkan/native/lessons/vk39_queue_properties/main.cpp](main.cpp#L26)，第 26—47 行；这是现有实现的局部摘录，末尾可能处于函数中间，不是可单独编译的完整程序。

```cpp
int runChapter(int argc, char** argv) {
    auto options = labOptions(argc, argv, 39);
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
| Vulkan 原生 | [vk39_queue_properties](README.md) |
| 对照后端 | [dx39_queue_properties](../../../../DirectX12/lessons/dx39_queue_properties/README.md) |

### 先预测，再验证

说明为什么同一 queue 上顺序提交，仍可能需要资源内存屏障。

参考推导：提交顺序不自动提供每一种访问之间所需的可见性或布局/状态变化。要从生产者与消费者推导依赖，不能用“排在后面”替代规则。

## 验证与边界

运行 `--smoke` 执行有限自动检查。数值实验读取实际 GPU 结果，图形实验检查非空图像并可查看读回。交互重建与性能还需额外证据；窗口 renderer 当前保留串行等待，双槽计算实验不会被描述为完整多帧呈现引擎。
