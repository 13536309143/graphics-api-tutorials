# dx53_vertex_layout · 顶点布局检验

[54 课目录](../../COURSE.md)

## 本课独立工程与关键注释

[main.cpp](main.cpp) · [本地辅助实现](support/) · [本课着色器](shaders/) · [CMakeLists.txt](CMakeLists.txt)

本目录包含独立课程源码，关键位置有中文注释。单独复制整个文件夹后可用 `cmake -S . -B build -A x64` 配置、`cmake --build build --config Debug` 构建；不需要其他章节源码。

## 目的与实际执行

VBV 给 GPU 地址、大小和 stride，input layout 用 POSITION/NORMAL/TEXCOORD 连接 HLSL 输入。语义名称与偏移分别检查，不能因为名字相同就忽略格式。实例数据还要声明独立的输入分类/步进。

本程序的 C++、辅助实现、着色器和构建脚本均位于本目录，关键步骤含中文注释。设备/API 检查、数值 compute 与图形渲染采用不同证据；不是每个程序都会弹出窗口。

## 构建和运行

```powershell
cmake --preset dx12
cmake --build --preset dx12-debug --target dx53_vertex_layout --parallel 4
.\DirectX12\build\bin\Debug\dx53_vertex_layout.exe
```

## 深入学习：数据、接口与执行

- [第 23 课：顶点语义、Stride 与 Offset](../../chapters/23_vertex_input/README.md)

### 本课的真实源码切片

来自 [DirectX12/lessons/dx53_vertex_layout/main.cpp](main.cpp#L40)，第 40—53 行；这是现有实现的局部摘录，末尾可能处于函数中间，不是可单独编译的完整程序。

```cpp
int runChapter(int argc, char** argv) {
    auto options = labOptions(argc, argv, 53);
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
| DirectX 12 | [dx53_vertex_layout](README.md) |
| 对照后端 | [vk53_vertex_layout](../../../Vulkan/native/lessons/vk53_vertex_layout/README.md) |

### 先预测，再验证

把某一个属性偏移故意改错，先预测影响的是形状、颜色还是 UV，再用 mesh viewer 查字节。

参考推导：位置错会改变几何，颜色/法线错影响插值或光照，UV 错影响采样。一次只改一项才能判断其责任。

## 验证与边界

运行 `--smoke` 执行有限自动检查。数值实验读取实际 GPU 结果，图形实验检查非空图像并可查看读回。交互重建与性能还需额外证据；窗口 renderer 当前保留串行等待，双槽计算实验不会被描述为完整多帧呈现引擎。

## 实际画面

![dx53_vertex_layout](../../../docs/previews/dx53_vertex_layout.png)
