# vk44_command_reuse · 命令存储安全重用

[54 课目录](../../COURSE.md)

## 本课独立工程与关键注释

[main.cpp](main.cpp) · [本地辅助实现](support/) · [本课着色器](shaders/) · [CMakeLists.txt](CMakeLists.txt)

本目录包含独立课程源码，关键位置有中文注释。单独复制整个文件夹后可用 `cmake -S . -B build -A x64` 配置、`cmake --build build --config Debug` 构建；不需要其他章节源码。

## 目的与实际执行

从与队列族匹配的 pool 分配 command buffer，完成后 reset pool，再录制新一代 fill。pool reset 与单独 buffer reset 的 flag/生命周期规则不同；本实验选择整池重置以展示最清楚的拥有关系。

本程序的 C++、辅助实现、着色器和构建脚本均位于本目录，关键步骤含中文注释。设备/API 检查、数值 compute 与图形渲染采用不同证据；不是每个程序都会弹出窗口。

## 构建和运行

```powershell
cmake --preset vulkan-native
cmake --build --preset vulkan-native-debug --target vk44_command_reuse --parallel 4
.\Vulkan\native\build\bin\Debug\vk44_command_reuse.exe
```

## 深入学习：数据、接口与执行

- [第 18 课：命令池、Allocator 与安全重置](../../chapters/18_command_storage/README.md)

### 本课的真实源码切片

来自 [Vulkan/native/lessons/vk44_command_reuse/main.cpp](main.cpp#L92)，第 92—113 行；这是现有实现的局部摘录，末尾可能处于函数中间，不是可单独编译的完整程序。

```cpp
int runChapter(int argc, char** argv) {
    auto options = labOptions(argc, argv, 44);
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
| Vulkan 原生 | [vk44_command_reuse](README.md) |
| 对照后端 | [dx44_command_reuse](../../../../DirectX12/lessons/dx44_command_reuse/README.md) |

### 先预测，再验证

四次录制分别写 41、42、43、44，怎样验证重置与执行链路？

参考推导：每代都等待其完成并核对全部结果，再允许下一次覆盖命令存储。仅最终值正确不能证明中间代次没有生命周期错误。

## 验证与边界

运行 `--smoke` 执行有限自动检查。数值实验读取实际 GPU 结果，图形实验检查非空图像并可查看读回。交互重建与性能还需额外证据；窗口 renderer 当前保留串行等待，双槽计算实验不会被描述为完整多帧呈现引擎。
