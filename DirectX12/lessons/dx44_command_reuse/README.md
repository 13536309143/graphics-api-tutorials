# dx44_command_reuse · 命令存储安全重用

[54 课目录](../../COURSE.md)

## 本课独立工程与关键注释

[main.cpp](main.cpp) · [本地辅助实现](support/) · [本课着色器](shaders/) · [CMakeLists.txt](CMakeLists.txt)

本目录包含独立课程源码，关键位置有中文注释。单独复制整个文件夹后可用 `cmake -S . -B build -A x64` 配置、`cmake --build build --config Debug` 构建；不需要其他章节源码。

## 目的与实际执行

CommandAllocator 保存录制存储，CommandList 提供录制接口。只有相关 GPU 使用结束后才能 reset allocator；command list 对象自身的 Reset 条件不应被误当成存储已安全复用。

本程序的 C++、辅助实现、着色器和构建脚本均位于本目录，关键步骤含中文注释。设备/API 检查、数值 compute 与图形渲染采用不同证据；不是每个程序都会弹出窗口。

## 构建和运行

```powershell
cmake --preset dx12
cmake --build --preset dx12-debug --target dx44_command_reuse --parallel 4
.\DirectX12\build\bin\Debug\dx44_command_reuse.exe
```

## 深入学习：数据、接口与执行

- [第 18 课：命令池、Allocator 与安全重置](../../chapters/18_command_storage/README.md)

### 本课的真实源码切片

来自 [DirectX12/lessons/dx44_command_reuse/main.cpp](main.cpp#L48)，第 48—64 行；这是现有实现的局部摘录，末尾可能处于函数中间，不是可单独编译的完整程序。

```cpp
int runChapter(int argc, char** argv) {
    auto options = labOptions(argc, argv, 44);
    DxDevice c(options.warp);
    {
        inspection(c, options);
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
| DirectX 12 | [dx44_command_reuse](README.md) |
| 对照后端 | [vk44_command_reuse](../../../Vulkan/native/lessons/vk44_command_reuse/README.md) |

### 先预测，再验证

四次录制分别写 41、42、43、44，怎样验证重置与执行链路？

参考推导：每代都等待其完成并核对全部结果，再允许下一次覆盖命令存储。仅最终值正确不能证明中间代次没有生命周期错误。

## 验证与边界

运行 `--smoke` 执行有限自动检查。数值实验读取实际 GPU 结果，图形实验检查非空图像并可查看读回。交互重建与性能还需额外证据；窗口 renderer 当前保留串行等待，双槽计算实验不会被描述为完整多帧呈现引擎。
