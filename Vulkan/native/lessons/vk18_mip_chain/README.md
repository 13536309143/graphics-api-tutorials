# vk18_mip_chain · GPU mip 链与逐级读回

[54 课目录](../../COURSE.md)

## 本课独立工程与关键注释

[main.cpp](main.cpp) · [本地辅助实现](support/) · [本课着色器](shaders/) · [CMakeLists.txt](CMakeLists.txt)

本目录包含独立课程源码，关键位置有中文注释。单独复制整个文件夹后可用 `cmake -S . -B build -A x64` 配置、`cmake --build build --config Debug` 构建；不需要其他章节源码。

## 目的与实际执行

copy-write 到 fragment-sampled-read 需要对应 stage/access 和布局转换。mip 生成逐级把前一级从 transfer-dst 切成 transfer-src，最终各级转 shader-read；不能仅在整个流程结束后随意改一个枚举。

查询格式的 blit/filter 支持，上传 level 0，逐级 transition 和 linear blit，再把所有级别转成 shader-read。每次只读取已经完成的前一级，最后复制每级结果用于校验。数值参考使用线性 UNORM。

本程序的 C++、辅助实现、着色器和构建脚本均位于本目录，关键步骤含中文注释。设备/API 检查、数值 compute 与图形渲染采用不同证据；不是每个程序都会弹出窗口。

## 构建和运行

```powershell
cmake --preset vulkan-native
cmake --build --preset vulkan-native-debug --target vk18_mip_chain --parallel 4
.\Vulkan\native\build\bin\Debug\vk18_mip_chain.exe
```

## 深入学习：数据、接口与执行

- [第 35 课：图像布局、资源状态与读写依赖](../../chapters/35_image_dependencies/README.md)
- [第 43 课：GPU Mip 链生成与逐级验收](../../chapters/43_mip_generation/README.md)

### 本课的真实源码切片

来自 [Vulkan/native/lessons/vk18_mip_chain/main.cpp](main.cpp#L244)，第 244—266 行；这是现有实现的局部摘录，末尾可能处于函数中间，不是可单独编译的完整程序。

```cpp
int runChapter(int argc, char** argv) {
    auto options = labOptions(argc, argv, 18);
    nativeShaders = options.directory / "shaders";
    validationErrors = 0;
    {
        NativeDevice owner(nullptr, false);
        const auto& c = owner.c;
        {
            GpuTexture texture(c, Pixels{64, 64, checker()}, true);
            texture.verify();
        }
    }
    std::cout << "Validation errors: " << validationErrors << '\n';
    return validationErrors ? 1 : 0;
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
| Vulkan 原生 | [vk18_mip_chain](README.md) |
| 对照后端 | [dx18_mip_chain](../../../../DirectX12/lessons/dx18_mip_chain/README.md) |

### 先预测，再验证

把纹理消费者从 pixel/fragment 改为 compute，哪些同步描述也要一起改变？

参考推导：消费者阶段/访问或 state 应反映 compute 读取。直接复用只针对片元的依赖不一定覆盖新的使用者。

把基图改为棋盘或分区整数颜色，为什么比任意照片更适合作为 mip 数值验收？

参考推导：可手算局部平均，并发现级数、偏移或尾部错误。图像“看着模糊了”不足以证明每一级都正确。

## 验证与边界

运行 `--smoke` 执行有限自动检查。数值实验读取实际 GPU 结果，图形实验检查非空图像并可查看读回。交互重建与性能还需额外证据；窗口 renderer 当前保留串行等待，双槽计算实验不会被描述为完整多帧呈现引擎。
