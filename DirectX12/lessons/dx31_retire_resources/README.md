# dx31_retire_resources · 完成后回收资源

[54 课目录](../../COURSE.md)

## 本课独立工程与关键注释

[main.cpp](main.cpp) · [本地辅助实现](support/) · [本课着色器](shaders/) · [CMakeLists.txt](CMakeLists.txt)

本目录包含独立课程源码，关键位置有中文注释。单独复制整个文件夹后可用 `cmake -S . -B build -A x64` 配置、`cmake --build build --config Debug` 构建；不需要其他章节源码。

## 目的与实际执行

ComPtr 只控制 COM 引用计数；最后一个引用释放时若 GPU 还在用资源，仍然错误。UPLOAD 的 CPU 字节也不能在 copy 未完成时覆盖。fence 完成值同时保护 resource、descriptor 范围和 command allocator 的回收。

本程序的 C++、辅助实现、着色器和构建脚本均位于本目录，关键步骤含中文注释。设备/API 检查、数值 compute 与图形渲染采用不同证据；不是每个程序都会弹出窗口。

## 构建和运行

```powershell
cmake --preset dx12
cmake --build --preset dx12-debug --target dx31_retire_resources --parallel 4
.\DirectX12\build\bin\Debug\dx31_retire_resources.exe
```

## 深入学习：数据、接口与执行

- [第 03 课：应用结构、RAII 与 GPU 使用期](../../chapters/03_lifetime/README.md)

### 本课的真实源码切片

来自 [DirectX12/lessons/dx31_retire_resources/main.cpp](main.cpp#L53)，第 53—71 行；这是现有实现的局部摘录，末尾可能处于函数中间，不是可单独编译的完整程序。

```cpp
int runChapter(int argc, char** argv) {
    auto options = labOptions(argc, argv, 31);
    DxDevice c(options.warp);
    {
        for (int i = 0; i < 4; ++i)
            stagingCopy(c);
        std::cout << "Retired 4 UPLOAD/DEFAULT/READBACK generations after fence completion\n";
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
| DirectX 12 | [dx31_retire_resources](README.md) |
| 对照后端 | [vk31_retire_resources](../../../Vulkan/native/lessons/vk31_retire_resources/README.md) |

### 先预测，再验证

将临时 upload 的回收点标到时间图上；指出“Execute/Submit 返回”之后还缺哪条证据。

参考推导：需要该资源最后一次使用所对应的 GPU 完成条件。提交返回仅说明工作已交给执行系统，不说明 copy 或 draw 已读完。

## 验证与边界

运行 `--smoke` 执行有限自动检查。数值实验读取实际 GPU 结果，图形实验检查非空图像并可查看读回。交互重建与性能还需额外证据；窗口 renderer 当前保留串行等待，双槽计算实验不会被描述为完整多帧呈现引擎。
