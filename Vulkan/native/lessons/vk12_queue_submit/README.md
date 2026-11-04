# vk12_queue_submit · 提交与完成条件

[54 课目录](../../COURSE.md)

## 目的与实际执行

begin/end command buffer 建立合法录制范围，QueueSubmit 接收它并关联 fence。实验填充 64 个 word，经 transfer-write→host-read 依赖后逐项核对。命令池的 family 必须匹配提交队列。

本程序有独立实验选择与验证路径，公共实现集中在 advanced_labs.cpp。设备/API 检查、数值 compute 与图形渲染采用不同证据；不是每个程序都会弹出窗口。

## 构建和运行

```powershell
cmake --preset vulkan-native
cmake --build --preset vulkan-native-debug --target vk12_queue_submit --parallel 4
.\Vulkan\native\build\bin\Debug\vk12_queue_submit.exe
```

## 深入学习：数据、接口与执行

- [第 19 课：命令录制、提交与可验证工作](../../chapters/19_record_submit/README.md)

### 本课的真实源码切片

来自 [Vulkan/native/common/advanced_labs.cpp](../../common/advanced_labs.cpp#L928)，第 928—971 行；这是现有实现的局部摘录，末尾可能处于函数中间，不是可单独编译的完整程序。

```cpp
int nativeLabMain(int argc, char** argv, int id) {
    auto options = labOptions(argc, argv, id);
    nativeShaders = options.directory / "shaders";
    if (id == 17 || id == 19 || id == 20 || id == 21 || id == 22 || id == 23 || id == 27 ||
        id == 29 || id == 34 || id == 35 || id == 36 || id == 41 || id == 43 || id == 49 ||
        id == 50 || id == 52 || id == 53)
        return graphicsLab(argc, argv, options);
    validationErrors = 0;
    if (id == 40) {
        surfaceInspection();
        return validationErrors ? 1 : 0;
    }
    {
        NativeDevice owner(nullptr, id == 26);
        const auto& c = owner.c;
        if (id >= 37)
            inspection(c, id);
        else if (id == 11)
            capabilities(c);
        else if (id == 12) {
            VkBufferResource b;
            b.init(c, 256, VK_BUFFER_USAGE_TRANSFER_DST_BIT);
            immediate(c, [&](VkCommandBuffer cmd) {
                vkCmdFillBuffer(cmd, b.buffer, 0, 256, 0x12345678);
                bufferDependency(cmd,
                                 b.buffer,
                                 VK_PIPELINE_STAGE_2_COPY_BIT | VK_PIPELINE_STAGE_2_CLEAR_BIT,
                                 VK_ACCESS_2_TRANSFER_WRITE_BIT,
                                 VK_PIPELINE_STAGE_2_HOST_BIT,
                                 VK_ACCESS_2_HOST_READ_BIT);
            });
            for (auto x : readWords(b, 64))
                require(x == 0x12345678, "Queue fill mismatch");
            std::cout << "Verified queue submission, fence and 64 filled words\n";
        } else if (id == 13)
            stagingCopy(c);
        else if (id == 18) {
            GpuTexture texture(c, Pixels{64, 64, checker()}, true);
            texture.verify();
        } else if (id == 24)
            frameRing(c);
        else if (id == 25)
            timestamps(c);
        else if (id == 26)
```

### 同主题跨 API 导航

| 路线 | 程序 |
| --- | --- |
| Vulkan 原生 | [vk12_queue_submit](README.md) |
| 对照后端 | [dx12_queue_submit](../../../../DirectX12/lessons/dx12_queue_submit/README.md) |

### 先预测，再验证

分别在录制结束、提交返回和完成等待之后设置 CPU 标记，说明哪一个允许读取结果。

参考推导：只有对应 GPU 完成条件成立且读取依赖正确时允许读取。前两个标记描述 CPU 进度，不证明 GPU 已写完。

## 验证与边界

运行 `--smoke` 执行有限自动检查。数值实验读取实际 GPU 结果，图形实验检查非空图像并可查看读回。交互重建与性能还需额外证据；窗口 renderer 当前保留串行等待，双槽计算实验不会被描述为完整多帧呈现引擎。
