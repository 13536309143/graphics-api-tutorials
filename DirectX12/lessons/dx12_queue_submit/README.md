# dx12_queue_submit · 提交与完成条件

[54 课目录](../../COURSE.md)

## 目的与实际执行

CreateCommandList 初次返回可录制对象；提交前 Close。执行列表本身没有返回“工作已完成”的 HRESULT，因此要用队列 signal/fence 等待。实验检查空提交的完成条件，后续拷贝实验再检查数据。

本程序有独立实验选择与验证路径，公共实现集中在 advanced_labs.cpp。设备/API 检查、数值 compute 与图形渲染采用不同证据；不是每个程序都会弹出窗口。

## 构建和运行

```powershell
cmake --preset dx12
cmake --build --preset dx12-debug --target dx12_queue_submit --parallel 4
.\DirectX12\build\bin\Debug\dx12_queue_submit.exe
```

## 深入学习：数据、接口与执行

- [第 19 课：命令录制、提交与可验证工作](../../chapters/19_record_submit/README.md)

### 本课的真实源码切片

来自 [DirectX12/common/advanced_labs.cpp](../../common/advanced_labs.cpp#L748)，第 748—791 行；这是现有实现的局部摘录，末尾可能处于函数中间，不是可单独编译的完整程序。

```cpp
int dxLabMain(int argc, char** argv, int id) {
    auto options = labOptions(argc, argv, id);
    if (id == 17 || id == 19 || id == 20 || id == 21 || id == 22 || id == 23 || id == 27 ||
        id == 29 || id == 34 || id == 35 || id == 36 || id == 41 || id == 43 || id == 49 ||
        id == 50 || id == 52 || id == 53)
        return graphicsLab(argc, argv, options);
    if (id == 40) {
        surfaceInspection(options);
        return 0;
    }
    DxDevice c(options.warp);
    if (id >= 37)
        inspection(c, options, id);
    else if (id == 11)
        capabilities(c);
    else if (id == 12) {
        c.begin();
        dxCheck(c.cmd->Close());
        ID3D12CommandList* lists[] = {c.cmd.Get()};
        c.queue->ExecuteCommandLists(1, lists);
        c.wait();
        require(c.fence->GetCompletedValue() >= c.value, "Queue fence did not complete");
        std::cout << "Verified command list close/submit and fence completion\n";
    } else if (id == 13)
        stagingCopy(c);
    else if (id == 18) {
        GpuTexture texture(c, options, Pixels{64, 64, checker()}, true);
        texture.verify();
    } else if (id == 24)
        frameRing(c, options);
    else if (id == 25)
        timestamps(c);
    else if (id == 26) {
        dxCheck(c.queue->Signal(c.fence.Get(), ++c.value));
        dxCheck(c.queue->Wait(c.fence.Get(), c.value));
        dxCheck(c.queue->Signal(c.fence.Get(), ++c.value));
        waitValue(c, c.value);
        require(c.fence->GetCompletedValue() >= 2, "Fence counter did not advance");
        std::cout << "Verified queue producer 1 -> consumer 2 fence chain\n";
    } else if (id == 31) {
        for (int i = 0; i < 4; ++i)
            stagingCopy(c);
        std::cout << "Retired 4 UPLOAD/DEFAULT/READBACK generations after fence completion\n";
    } else if (id == 32) {
```

### 同主题跨 API 导航

| 路线 | 程序 |
| --- | --- |
| DirectX 12 | [dx12_queue_submit](README.md) |
| 对照后端 | [vk12_queue_submit](../../../Vulkan/native/lessons/vk12_queue_submit/README.md) |

### 先预测，再验证

分别在录制结束、提交返回和完成等待之后设置 CPU 标记，说明哪一个允许读取结果。

参考推导：只有对应 GPU 完成条件成立且读取依赖正确时允许读取。前两个标记描述 CPU 进度，不证明 GPU 已写完。

## 验证与边界

运行 `--smoke` 执行有限自动检查。数值实验读取实际 GPU 结果，图形实验检查非空图像并可查看读回。交互重建与性能还需额外证据；窗口 renderer 当前保留串行等待，双槽计算实验不会被描述为完整多帧呈现引擎。
