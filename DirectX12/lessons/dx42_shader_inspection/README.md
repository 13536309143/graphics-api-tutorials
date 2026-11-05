# dx42_shader_inspection · 着色器产物检查

[54 课目录](../../COURSE.md)

## 本课独立工程与关键注释

[main.cpp](main.cpp) · [本地辅助实现](support/) · [本课着色器](shaders/) · [CMakeLists.txt](CMakeLists.txt)

本目录包含独立课程源码，关键位置有中文注释。单独复制整个文件夹后可用 `cmake -S . -B build -A x64` 配置、`cmake --build build --config Debug` 构建；不需要其他章节源码。

## 目的与实际执行

D3DCompiler 生成 SM5.1 字节码；实验通过反射确认 ComputeMain 的工作组为 64×1×1 并检查资源绑定。Debug 产物可能带不同调试数据，管线缓存重建应复用完全一致的字节码，不能假定同源码重编译必然字节相同。 本例使用 SM5.x 的 ID3D11ShaderReflection 反射接口，这不表示渲染后端改成了 D3D11。

本程序的 C++、辅助实现、着色器和构建脚本均位于本目录，关键步骤含中文注释。设备/API 检查、数值 compute 与图形渲染采用不同证据；不是每个程序都会弹出窗口。

## 构建和运行

```powershell
cmake --preset dx12
cmake --build --preset dx12-debug --target dx42_shader_inspection --parallel 4
.\DirectX12\build\bin\Debug\dx42_shader_inspection.exe
```

## 深入学习：数据、接口与执行

- [第 13 课：着色器编译、模块与反射](../../chapters/13_shader_modules/README.md)

### 本课的真实源码切片

来自 [DirectX12/lessons/dx42_shader_inspection/main.cpp](main.cpp#L55)，第 55—71 行；这是现有实现的局部摘录，末尾可能处于函数中间，不是可单独编译的完整程序。

```cpp
int runChapter(int argc, char** argv) {
    auto options = labOptions(argc, argv, 42);
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
| DirectX 12 | [dx42_shader_inspection](README.md) |
| 对照后端 | [vk42_shader_inspection](../../../Vulkan/native/lessons/vk42_shader_inspection/README.md) |

### 先预测，再验证

将 shader 的 local size 改为 32，但 host 的组数不改，预测覆盖范围和验证结果。

参考推导：总调用数量减半，尾部不会按原契约写入。反射/声明、dispatch 组数、缓冲容量和 CPU 检查必须共同更新。

## 验证与边界

运行 `--smoke` 执行有限自动检查。数值实验读取实际 GPU 结果，图形实验检查非空图像并可查看读回。交互重建与性能还需额外证据；窗口 renderer 当前保留串行等待，双槽计算实验不会被描述为完整多帧呈现引擎。
