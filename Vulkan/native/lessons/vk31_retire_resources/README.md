# vk31_retire_resources · 完成后回收资源

[54 课目录](../../COURSE.md)

## 本课独立工程与关键注释

[main.cpp](main.cpp) · [本地辅助实现](support/) · [本课着色器](shaders/) · [CMakeLists.txt](CMakeLists.txt)

本目录包含独立课程源码，关键位置有中文注释。单独复制整个文件夹后可用 `cmake -S . -B build -A x64` 配置、`cmake --build build --config Debug` 构建；不需要其他章节源码。

## 目的与实际执行

Buffer 销毁后才能释放它绑定的 memory；依赖 Device 的对象先于 Device 结束。command pool、descriptor pool 和 image view 也各有使用期。vkDeviceWaitIdle 可作为教学退出边界，但逐资源 fence/timeline 可以更精确地回收。

本程序的 C++、辅助实现、着色器和构建脚本均位于本目录，关键步骤含中文注释。设备/API 检查、数值 compute 与图形渲染采用不同证据；不是每个程序都会弹出窗口。

## 构建和运行

```powershell
cmake --preset vulkan-native
cmake --build --preset vulkan-native-debug --target vk31_retire_resources --parallel 4
.\Vulkan\native\build\bin\Debug\vk31_retire_resources.exe
```

## 深入学习：数据、接口与执行

- [第 03 课：应用结构、RAII 与 GPU 使用期](../../chapters/03_lifetime/README.md)

### 本课的真实源码切片

来自 [Vulkan/native/lessons/vk31_retire_resources/main.cpp](main.cpp#L79)，第 79—102 行；这是现有实现的局部摘录，末尾可能处于函数中间，不是可单独编译的完整程序。

```cpp
int runChapter(int argc, char** argv) {
    auto options = labOptions(argc, argv, 31);
    nativeShaders = options.directory / "shaders";
    validationErrors = 0;
    {
        NativeDevice owner(nullptr, false);
        const auto& c = owner.c;
        {
            for (int i = 0; i < 4; ++i)
                stagingCopy(c);
            std::cout << "Retired 4 upload/device/readback generations after completion\n";
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
| Vulkan 原生 | [vk31_retire_resources](README.md) |
| 对照后端 | [dx31_retire_resources](../../../../DirectX12/lessons/dx31_retire_resources/README.md) |

### 先预测，再验证

将临时 upload 的回收点标到时间图上；指出“Execute/Submit 返回”之后还缺哪条证据。

参考推导：需要该资源最后一次使用所对应的 GPU 完成条件。提交返回仅说明工作已交给执行系统，不说明 copy 或 draw 已读完。

## 验证与边界

运行 `--smoke` 执行有限自动检查。数值实验读取实际 GPU 结果，图形实验检查非空图像并可查看读回。交互重建与性能还需额外证据；窗口 renderer 当前保留串行等待，双槽计算实验不会被描述为完整多帧呈现引擎。
