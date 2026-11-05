# dx45_descriptor_update · 资源引用替换

[54 课目录](../../COURSE.md)

## 本课独立工程与关键注释

[main.cpp](main.cpp) · [本地辅助实现](support/) · [本课着色器](shaders/) · [CMakeLists.txt](CMakeLists.txt)

本目录包含独立课程源码，关键位置有中文注释。单独复制整个文件夹后可用 `cmake -S . -B build -A x64` 配置、`cmake --build build --config Debug` 构建；不需要其他章节源码。

## 目的与实际执行

计算实验用 root UAV 地址替换资源引用，纹理实验则通过 shader-visible heap 与 descriptor table 展示 heap 模型。二者不是同一接口，课程明确标注；CPU descriptor 的构建与 GPU handle 的绑定各有位置。

本程序的 C++、辅助实现、着色器和构建脚本均位于本目录，关键步骤含中文注释。设备/API 检查、数值 compute 与图形渲染采用不同证据；不是每个程序都会弹出窗口。

## 构建和运行

```powershell
cmake --preset dx12
cmake --build --preset dx12-debug --target dx45_descriptor_update --parallel 4
.\DirectX12\build\bin\Debug\dx45_descriptor_update.exe
```

## 深入学习：数据、接口与执行

- [第 30 课：描述符池、Heap 与资源引用更新](../../chapters/30_descriptor_instances/README.md)

### 本课的真实源码切片

来自 [DirectX12/lessons/dx45_descriptor_update/main.cpp](main.cpp#L167)，第 167—183 行；这是现有实现的局部摘录，末尾可能处于函数中间，不是可单独编译的完整程序。

```cpp
int runChapter(int argc, char** argv) {
    auto options = labOptions(argc, argv, 45);
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
| DirectX 12 | [dx45_descriptor_update](README.md) |
| 对照后端 | [vk45_descriptor_update](../../../Vulkan/native/lessons/vk45_descriptor_update/README.md) |

### 先预测，再验证

修改绑定到 B 后，为什么还要检查 A？

参考推导：这能发现 CPU 误写旧资源、错误绑定或读回来源混淆。只看 B 的某个值可能被旧内容偶然满足。

## 验证与边界

运行 `--smoke` 执行有限自动检查。数值实验读取实际 GPU 结果，图形实验检查非空图像并可查看读回。交互重建与性能还需额外证据；窗口 renderer 当前保留串行等待，双槽计算实验不会被描述为完整多帧呈现引擎。
