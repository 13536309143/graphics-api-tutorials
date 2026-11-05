# vk26_timeline_counter · 计数同步与生产消费

[54 课目录](../../COURSE.md)

## 本课独立工程与关键注释

[main.cpp](main.cpp) · [本地辅助实现](support/) · [本课着色器](shaders/) · [CMakeLists.txt](CMakeLists.txt)

本目录包含独立课程源码，关键位置有中文注释。单独复制整个文件夹后可用 `cmake -S . -B build -A x64` 配置、`cmake --build build --config Debug` 构建；不需要其他章节源码。

## 目的与实际执行

设备额外查询并启用 timelineSemaphore。第一次提交 signal=1，第二次 wait=1/signal=2，host 等到 2 并读取 counter。使用 submit2 的阶段范围明确，不把 timeline semaphore 当作交换链 binary semaphore 的直接替代。

本程序的 C++、辅助实现、着色器和构建脚本均位于本目录，关键步骤含中文注释。设备/API 检查、数值 compute 与图形渲染采用不同证据；不是每个程序都会弹出窗口。

## 构建和运行

```powershell
cmake --preset vulkan-native
cmake --build --preset vulkan-native-debug --target vk26_timeline_counter --parallel 4
.\Vulkan\native\build\bin\Debug\vk26_timeline_counter.exe
```

## 深入学习：数据、接口与执行

- [第 50 课：Timeline、Fence 计数与生产消费链](../../chapters/50_timeline/README.md)

### 本课的真实源码切片

来自 [Vulkan/native/lessons/vk26_timeline_counter/main.cpp](main.cpp#L51)，第 51—72 行；这是现有实现的局部摘录，末尾可能处于函数中间，不是可单独编译的完整程序。

```cpp
int runChapter(int argc, char** argv) {
    auto options = labOptions(argc, argv, 26);
    nativeShaders = options.directory / "shaders";
    validationErrors = 0;
    {
        NativeDevice owner(nullptr, true);
        const auto& c = owner.c;
        {
            timelineCounter(c);
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
| Vulkan 原生 | [vk26_timeline_counter](README.md) |
| 对照后端 | [dx26_timeline_counter](../../../../DirectX12/lessons/dx26_timeline_counter/README.md) |

### 先预测，再验证

说明 host 等 counter=2 与 GPU queue 等 counter=1 的区别。

参考推导：前者阻塞 CPU 到某进度，后者让 GPU 工作在依赖成立后执行，CPU 可以继续录制其他工作。等待对象相同不意味着阻塞同一执行主体。

## 验证与边界

运行 `--smoke` 执行有限自动检查。数值实验读取实际 GPU 结果，图形实验检查非空图像并可查看读回。交互重建与性能还需额外证据；窗口 renderer 当前保留串行等待，双槽计算实验不会被描述为完整多帧呈现引擎。
