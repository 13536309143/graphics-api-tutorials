# dx16_push_constants · 小常量计算

[54 课目录](../../COURSE.md)

## 本课独立工程与关键注释

[main.cpp](main.cpp) · [本地辅助实现](support/) · [本课着色器](shaders/) · [CMakeLists.txt](CMakeLists.txt)

本目录包含独立课程源码，关键位置有中文注释。单独复制整个文件夹后可用 `cmake -S . -B build -A x64` 配置、`cmake --build build --config Debug` 构建；不需要其他章节源码。

## 目的与实际执行

四个 root constants 映射 b0，root UAV 映射 u0，root CBV 映射 b1。root parameter 的数组下标与 HLSL register 不是一套编号；更新 root constants 使用 DWORD 数而不是字节数。

root signature 组合 root constants、root descriptor 和 descriptor table。parameter 2 可以指 t0，register space 又是独立维度。显式说明接口约定，才能检查 shader 与调用是否兼容。

root constants 使用 32 位值数量与偏移，对应 HLSL cbuffer。20 DWORD 的 Draw 与 4 DWORD 的 Operation 不同；root signature 总成本也限制接口设计。

本程序的 C++、辅助实现、着色器和构建脚本均位于本目录，关键步骤含中文注释。设备/API 检查、数值 compute 与图形渲染采用不同证据；不是每个程序都会弹出窗口。

## 构建和运行

```powershell
cmake --preset dx12
cmake --build --preset dx12-debug --target dx16_push_constants --parallel 4
.\DirectX12\build\bin\Debug\dx16_push_constants.exe
```

## 深入学习：数据、接口与执行

- [第 16 课：Pipeline Layout、根签名与小常量](../../chapters/16_pipeline_interface/README.md)
- [第 29 课：描述符布局与资源接口声明](../../chapters/29_descriptor_layout/README.md)
- [第 32 课：Push/Root Constants 的单位与边界](../../chapters/32_per_draw_constants/README.md)

### 本课的真实源码切片

来自 [DirectX12/lessons/dx16_push_constants/main.cpp](main.cpp#L136)，第 136—172 行；这是现有实现的局部摘录，末尾可能处于函数中间，不是可单独编译的完整程序。

```cpp
int runChapter(int argc, char** argv) {
    auto options = labOptions(argc, argv, 16);
    DxDevice c(options.warp);
    {
        ComputeKernel kernel(c, options);
        uint32_t mode = ((0));
        uint32_t count = (128);
        for (uint32_t slot = 0; slot < ((1u)); ++slot) {
            uint32_t factor = (9), bias = (17);
            kernel.execute(mode, count, factor, bias, slot);
            auto result = readBuffer(kernel.readback.Get(), mode == 2 ? (count + 63) / 64 : count);
            if (mode == 2) {
                auto sum = std::accumulate(result.begin(), result.end(), 0u);
                require(sum == count * (count + 1) / 2, "Shared reduction result mismatch");
            } else {
                if (mode == 1) {
                    factor = slot == 0 ? 2 : slot == 1 ? 5 : 7;
                    bias = slot == 0 ? 3 : slot == 1 ? 11 : 13;
                }
                for (uint32_t i = 0; i < count; ++i)
                    require(result[i] == i * factor + bias, "Compute/root-binding mismatch");
            }
        }
        std::cout << "Verified experiment " << 16 << ", items=" << count
                  << ", constant stride=256\n";
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
| DirectX 12 | [dx16_push_constants](README.md) |
| 对照后端 | [vk16_push_constants](../../../Vulkan/native/lessons/vk16_push_constants/README.md) |

### 先预测，再验证

把 factor/bias 改为 9/17，如何证明 GPU 确实收到了新常量？

参考推导：逐项检查 result[i]=9*i+17，而不是只看 CPU 断点中的变量。结果错时核对参数顺序、大小、阶段和入口。

为一个每帧相机、每材质纹理、每对象 model 的场景设计更新频率划分。

参考推导：按帧更新 Frame，按材质选择纹理引用，按 draw 更新 model 小常量。不要每 draw 重建全部布局，也不把大型场景数据塞进 push/root constants。

加入第二个对象，仅改变它的 model，说明不需要重建哪些长期对象。

参考推导：静态几何、纹理、layout 和同一兼容 pipeline 可以复用，只在对应 draw 前更新小常量。更改不可动态状态或 shader 才可能要求另一 pipeline。

## 验证与边界

运行 `--smoke` 执行有限自动检查。数值实验读取实际 GPU 结果，图形实验检查非空图像并可查看读回。交互重建与性能还需额外证据；窗口 renderer 当前保留串行等待，双槽计算实验不会被描述为完整多帧呈现引擎。
