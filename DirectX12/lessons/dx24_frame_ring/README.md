# dx24_frame_ring · 双帧计算资源环

[54 课目录](../../COURSE.md)

## 本课独立工程与关键注释

[main.cpp](main.cpp) · [本地辅助实现](support/) · [本课着色器](shaders/) · [CMakeLists.txt](CMakeLists.txt)

本目录包含独立课程源码，关键位置有中文注释。单独复制整个文件夹后可用 `cmake -S . -B build -A x64` 配置、`cmake --build build --config Debug` 构建；不需要其他章节源码。

## 目的与实际执行

每槽保存 allocator、command list、资源和完成值。queue Signal 记录本槽提交，下一次重用前 waitValue；一个 ID3D12Fence 通过不同值表达多个完成点。

本程序的 C++、辅助实现、着色器和构建脚本均位于本目录，关键步骤含中文注释。设备/API 检查、数值 compute 与图形渲染采用不同证据；不是每个程序都会弹出窗口。

## 构建和运行

```powershell
cmake --preset dx12
cmake --build --preset dx12-debug --target dx24_frame_ring --parallel 4
.\DirectX12\build\bin\Debug\dx24_frame_ring.exe
```

## 深入学习：数据、接口与执行

- [第 21 课：两帧资源环与在途使用期](../../chapters/21_frames_in_flight/README.md)

### 本课的真实源码切片

来自 [DirectX12/lessons/dx24_frame_ring/main.cpp](main.cpp#L190)，第 190—206 行；这是现有实现的局部摘录，末尾可能处于函数中间，不是可单独编译的完整程序。

```cpp
int runChapter(int argc, char** argv) {
    auto options = labOptions(argc, argv, 24);
    DxDevice c(options.warp);
    {
        frameRing(c, options);
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
| DirectX 12 | [dx24_frame_ring](README.md) |
| 对照后端 | [vk24_frame_ring](../../../Vulkan/native/lessons/vk24_frame_ring/README.md) |

### 先预测，再验证

把槽数改为三，列出所有必须一起改变的数组和完成条件。

参考推导：命令存储、可变缓冲、每槽记录与最终等待均需对应三份。交换链图像数量不必因此变三，imageIndex 仍来自呈现系统。

## 验证与边界

运行 `--smoke` 执行有限自动检查。数值实验读取实际 GPU 结果，图形实验检查非空图像并可查看读回。交互重建与性能还需额外证据；窗口 renderer 当前保留串行等待，双槽计算实验不会被描述为完整多帧呈现引擎。
