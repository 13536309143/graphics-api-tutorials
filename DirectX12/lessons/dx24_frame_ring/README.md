# dx24_frame_ring · 双帧计算资源环

[54 课目录](../../COURSE.md)

## 目的与实际执行

每槽保存 allocator、command list、资源和完成值。queue Signal 记录本槽提交，下一次重用前 waitValue；一个 ID3D12Fence 通过不同值表达多个完成点。

本程序有独立实验选择与验证路径，公共实现集中在 advanced_labs.cpp。设备/API 检查、数值 compute 与图形渲染采用不同证据；不是每个程序都会弹出窗口。

## 构建和运行

```powershell
cmake --preset dx12
cmake --build --preset dx12-debug --target dx24_frame_ring --parallel 4
.\DirectX12\build\bin\Debug\dx24_frame_ring.exe
```

## 深入学习：数据、接口与执行

- [第 21 课：两帧资源环与在途使用期](../../chapters/21_frames_in_flight/README.md)

### 本课的真实源码切片

来自 [DirectX12/common/advanced_labs.cpp](../../common/advanced_labs.cpp#L167)，第 167—210 行；这是现有实现的局部摘录，末尾可能处于函数中间，不是可单独编译的完整程序。

```cpp
void frameRing(DxDevice& c, const LabOptions& options) {
    ComputeKernel kernels[2] = {ComputeKernel(c, options), ComputeKernel(c, options)};
    ComPtr<ID3D12CommandAllocator> allocators[2];
    ComPtr<ID3D12GraphicsCommandList> commands[2];
    UINT64 completed[2]{};
    int previous[2] = {-1, -1};
    for (uint32_t i = 0; i < 2; ++i) {
        dxCheck(c.device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
                                                 IID_PPV_ARGS(&allocators[i])));
        dxCheck(c.device->CreateCommandList(0,
                                            D3D12_COMMAND_LIST_TYPE_DIRECT,
                                            allocators[i].Get(),
                                            nullptr,
                                            IID_PPV_ARGS(&commands[i])));
        dxCheck(commands[i]->Close());
    }
    for (uint32_t frame = 0; frame < 8; ++frame) {
        uint32_t slot = frame % 2;
        waitValue(c, completed[slot]);
        if (previous[slot] >= 0) {
            auto result = readBuffer(kernels[slot].readback.Get(), 257);
            for (uint32_t i = 0; i < 257; ++i)
                require(result[i] == i * 3 + uint32_t(previous[slot]),
                        "In-flight slot overwritten");
        }
        dxCheck(allocators[slot]->Reset());
        dxCheck(commands[slot]->Reset(allocators[slot].Get(), nullptr));
        kernels[slot].record(commands[slot].Get(), 0, 257, 3, frame);
        dxCheck(commands[slot]->Close());
        ID3D12CommandList* lists[] = {commands[slot].Get()};
        c.queue->ExecuteCommandLists(1, lists);
        completed[slot] = ++c.value;
        dxCheck(c.queue->Signal(c.fence.Get(), completed[slot]));
        previous[slot] = int(frame);
    }
    for (uint32_t slot = 0; slot < 2; ++slot) {
        waitValue(c, completed[slot]);
        auto result = readBuffer(kernels[slot].readback.Get(), 257);
        for (uint32_t i = 0; i < 257; ++i)
            require(result[i] == i * 3 + uint32_t(previous[slot]), "Final slot mismatch");
    }
    std::cout << "Verified 8 submissions using 2 independent allocator/buffer slots\n";
}
void timestamps(DxDevice& c) {
```

### 同主题跨 API 导航

| 路线 | 程序 |
| --- | --- |
| DirectX 12 | [dx24_frame_ring](README.md) |
| 对照后端 | [vk24_frame_ring](../../../Vulkan/native/lessons/vk24_frame_ring/README.md) |

### 先预测，再验证

把槽数改为三，列出所有必须一起改变的数组和完成条件。

参考推导：命令存储、可变缓冲、每槽记录与最终等待均需对应三份。交换链图像数量不必因此变三，imageIndex 仍来自呈现系统。

## 验证与边界

运行 `--smoke` 执行有限自动检查。数值实验读取实际 GPU 结果，图形实验检查非空图像并可查看读回。交互重建与性能还需额外证据；窗口 renderer 当前保留串行等待，双槽计算实验不会被描述为完整多帧呈现引擎。
