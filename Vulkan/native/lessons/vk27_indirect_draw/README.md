# vk27_indirect_draw · 间接绘制

[54 课目录](../../COURSE.md)

## 本课独立工程与关键注释

[main.cpp](main.cpp) · [本地辅助实现](support/) · [本课着色器](shaders/) · [CMakeLists.txt](CMakeLists.txt)

本目录包含独立课程源码，关键位置有中文注释。单独复制整个文件夹后可用 `cmake -S . -B build -A x64` 配置、`cmake --build build --config Debug` 构建；不需要其他章节源码。

## 目的与实际执行

VkDrawIndirectCommand 的四个 uint 字段与 stride 匹配，缓冲包含 INDIRECT_BUFFER usage，vkCmdDrawIndirect 读取参数。若以后 compute 写参数，需要 compute-write→draw-indirect-read 的依赖。

本程序的 C++、辅助实现、着色器和构建脚本均位于本目录，关键步骤含中文注释。设备/API 检查、数值 compute 与图形渲染采用不同证据；不是每个程序都会弹出窗口。

## 构建和运行

```powershell
cmake --preset vulkan-native
cmake --build --preset vulkan-native-debug --target vk27_indirect_draw --parallel 4
.\Vulkan\native\build\bin\Debug\vk27_indirect_draw.exe
```

## 深入学习：数据、接口与执行

- [第 51 课：间接参数、命令签名与可扩展提交](../../chapters/51_indirect/README.md)

### 本课的真实源码切片

来自 [Vulkan/native/lessons/vk27_indirect_draw/main.cpp](main.cpp#L42)，第 42—56 行；这是现有实现的局部摘录，末尾可能处于函数中间，不是可单独编译的完整程序。

```cpp
int runChapter(int argc, char** argv) {
    auto options = labOptions(argc, argv, 27);
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
| Vulkan 原生 | [vk27_indirect_draw](README.md) |
| 对照后端 | [dx27_indirect_draw](../../../../DirectX12/lessons/dx27_indirect_draw/README.md) |

### 先预测，再验证

把 vertex count 改为零，预测为何应该没有三角形，并说明它与缓冲为空的区别。

参考推导：合法参数要求零工作，与资源或绑定无效不是一回事。恢复 count 后应重新出现；错误句柄不应该被当作“禁用绘制”手段。

## 验证与边界

运行 `--smoke` 执行有限自动检查。数值实验读取实际 GPU 结果，图形实验检查非空图像并可查看读回。交互重建与性能还需额外证据；窗口 renderer 当前保留串行等待，双槽计算实验不会被描述为完整多帧呈现引擎。

## 实际画面

![vk27_indirect_draw](../../../../docs/previews/vk27_indirect_draw.png)
