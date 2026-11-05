# dx18_mip_chain · GPU mip 链与逐级读回

[54 课目录](../../COURSE.md)

## 本课独立工程与关键注释

[main.cpp](main.cpp) · [本地辅助实现](support/) · [本课着色器](shaders/) · [CMakeLists.txt](CMakeLists.txt)

本目录包含独立课程源码，关键位置有中文注释。单独复制整个文件夹后可用 `cmake -S . -B build -A x64` 配置、`cmake --build build --config Debug` 构建；不需要其他章节源码。

## 目的与实际执行

legacy transition barrier 表达 COPY_DEST、UAV、shader resource 等用途转换。compute mip 每级有自己的 subresource state；UAV 写后下级采样的依赖由对应转换建立。跨队列还需 fence Signal/Wait。

GPU 计算使用 SRV 指前一级、UAV 指后一级，8×8 工作组平均四个 texel。每级 descriptor 与 subresource state 分开管理，最后转可采样。sRGB 材质链另用正确线性平均的 CPU 路径，未把编码值简单平均冒充正确颜色 mip。

本程序的 C++、辅助实现、着色器和构建脚本均位于本目录，关键步骤含中文注释。设备/API 检查、数值 compute 与图形渲染采用不同证据；不是每个程序都会弹出窗口。

## 构建和运行

```powershell
cmake --preset dx12
cmake --build --preset dx12-debug --target dx18_mip_chain --parallel 4
.\DirectX12\build\bin\Debug\dx18_mip_chain.exe
```

## 深入学习：数据、接口与执行

- [第 35 课：图像布局、资源状态与读写依赖](../../chapters/35_image_dependencies/README.md)
- [第 43 课：GPU Mip 链生成与逐级验收](../../chapters/43_mip_generation/README.md)

### 本课的真实源码切片

来自 [DirectX12/lessons/dx18_mip_chain/main.cpp](main.cpp#L265)，第 265—282 行；这是现有实现的局部摘录，末尾可能处于函数中间，不是可单独编译的完整程序。

```cpp
int runChapter(int argc, char** argv) {
    auto options = labOptions(argc, argv, 18);
    DxDevice c(options.warp);
    {
        GpuTexture texture(c, options, Pixels{64, 64, checker()}, true);
        texture.verify();
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
| DirectX 12 | [dx18_mip_chain](README.md) |
| 对照后端 | [vk18_mip_chain](../../../Vulkan/native/lessons/vk18_mip_chain/README.md) |

### 先预测，再验证

把纹理消费者从 pixel/fragment 改为 compute，哪些同步描述也要一起改变？

参考推导：消费者阶段/访问或 state 应反映 compute 读取。直接复用只针对片元的依赖不一定覆盖新的使用者。

把基图改为棋盘或分区整数颜色，为什么比任意照片更适合作为 mip 数值验收？

参考推导：可手算局部平均，并发现级数、偏移或尾部错误。图像“看着模糊了”不足以证明每一级都正确。

## 验证与边界

运行 `--smoke` 执行有限自动检查。数值实验读取实际 GPU 结果，图形实验检查非空图像并可查看读回。交互重建与性能还需额外证据；窗口 renderer 当前保留串行等待，双槽计算实验不会被描述为完整多帧呈现引擎。
