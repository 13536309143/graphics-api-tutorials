# vk29_gpu_particles · GPU 生成顶点与绘制

[54 课目录](../../COURSE.md)

## 本课独立工程与关键注释

[main.cpp](main.cpp) · [本地辅助实现](support/) · [本课着色器](shaders/) · [CMakeLists.txt](CMakeLists.txt)

本目录包含独立课程源码，关键位置有中文注释。单独复制整个文件夹后可用 `cmake -S . -B build -A x64` 配置、`cmake --build build --config Debug` 构建；不需要其他章节源码。

## 目的与实际执行

同一 buffer 具有 STORAGE_BUFFER 和 VERTEX_BUFFER usage。计算写入通过 compute-storage-write→vertex-attribute-read 依赖连接绘制。scalar 形式的 storage Vertex 与输入 32 字节 stride 一致，避免 std430 vec3 对齐误配。 直接裁剪坐标在绘制的 Frame 里显式翻转 Y，使数学粒子位置与 DX12 视口映射一致；GPU 生成的数据仍保持同一布局。

本程序的 C++、辅助实现、着色器和构建脚本均位于本目录，关键步骤含中文注释。设备/API 检查、数值 compute 与图形渲染采用不同证据；不是每个程序都会弹出窗口。

## 构建和运行

```powershell
cmake --preset vulkan-native
cmake --build --preset vulkan-native-debug --target vk29_gpu_particles --parallel 4
.\Vulkan\native\build\bin\Debug\vk29_gpu_particles.exe
```

## 深入学习：数据、接口与执行

- [第 48 课：计算生成顶点并直接绘制](../../chapters/48_compute_graphics/README.md)

### 本课的真实源码切片

来自 [Vulkan/native/lessons/vk29_gpu_particles/main.cpp](main.cpp#L166)，第 166—180 行；这是现有实现的局部摘录，末尾可能处于函数中间，不是可单独编译的完整程序。

```cpp
int runChapter(int argc, char** argv) {
    auto options = labOptions(argc, argv, 29);
    nativeShaders = options.directory / "shaders";
    {
        return graphicsLab(argc, argv, options);
    }
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
| Vulkan 原生 | [vk29_gpu_particles](README.md) |
| 对照后端 | [dx29_gpu_particles](../../../../DirectX12/lessons/dx29_gpu_particles/README.md) |

### 先预测，再验证

把粒子数改为 97，需要检查哪些容量与 dispatch/guard？

参考推导：顶点数变为 291，GPU buffer 与 CPU 侧 draw count 元数据都要扩大；组数向上取整，shader 以 count*3 为上界。位置仍由 GPU 生成。

## 验证与边界

运行 `--smoke` 执行有限自动检查。数值实验读取实际 GPU 结果，图形实验检查非空图像并可查看读回。交互重建与性能还需额外证据；窗口 renderer 当前保留串行等待，双槽计算实验不会被描述为完整多帧呈现引擎。

## 实际画面

![vk29_gpu_particles](../../../../docs/previews/vk29_gpu_particles.png)
