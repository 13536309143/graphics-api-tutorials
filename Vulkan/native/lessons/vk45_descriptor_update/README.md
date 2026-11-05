# vk45_descriptor_update · 资源引用替换

[54 课目录](../../COURSE.md)

## 本课独立工程与关键注释

[main.cpp](main.cpp) · [本地辅助实现](support/) · [本课着色器](shaders/) · [CMakeLists.txt](CMakeLists.txt)

本目录包含独立课程源码，关键位置有中文注释。单独复制整个文件夹后可用 `cmake -S . -B build -A x64` 配置、`cmake --build build --config Debug` 构建；不需要其他章节源码。

## 目的与实际执行

pool 按描述符类型与数量提供分配容量，set 从 pool 分配。update 替换 binding 0 的 storage buffer，必须等原使用完成并遵循更新规则；普通 descriptor set 不能在 GPU 使用期间随意改写。

本程序的 C++、辅助实现、着色器和构建脚本均位于本目录，关键步骤含中文注释。设备/API 检查、数值 compute 与图形渲染采用不同证据；不是每个程序都会弹出窗口。

## 构建和运行

```powershell
cmake --preset vulkan-native
cmake --build --preset vulkan-native-debug --target vk45_descriptor_update --parallel 4
.\Vulkan\native\build\bin\Debug\vk45_descriptor_update.exe
```

## 深入学习：数据、接口与执行

- [第 30 课：描述符池、Heap 与资源引用更新](../../chapters/30_descriptor_instances/README.md)

### 本课的真实源码切片

来自 [Vulkan/native/lessons/vk45_descriptor_update/main.cpp](main.cpp#L186)，第 186—207 行；这是现有实现的局部摘录，末尾可能处于函数中间，不是可单独编译的完整程序。

```cpp
int runChapter(int argc, char** argv) {
    auto options = labOptions(argc, argv, 45);
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
| Vulkan 原生 | [vk45_descriptor_update](README.md) |
| 对照后端 | [dx45_descriptor_update](../../../../DirectX12/lessons/dx45_descriptor_update/README.md) |

### 先预测，再验证

修改绑定到 B 后，为什么还要检查 A？

参考推导：这能发现 CPU 误写旧资源、错误绑定或读回来源混淆。只看 B 的某个值可能被旧内容偶然满足。

## 验证与边界

运行 `--smoke` 执行有限自动检查。数值实验读取实际 GPU 结果，图形实验检查非空图像并可查看读回。交互重建与性能还需额外证据；窗口 renderer 当前保留串行等待，双槽计算实验不会被描述为完整多帧呈现引擎。
