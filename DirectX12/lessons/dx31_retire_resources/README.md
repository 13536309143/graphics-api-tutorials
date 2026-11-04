# dx31_retire_resources · 完成后回收资源

[54 课目录](../../COURSE.md)

## 目的与实际执行

ComPtr 只控制 COM 引用计数；最后一个引用释放时若 GPU 还在用资源，仍然错误。UPLOAD 的 CPU 字节也不能在 copy 未完成时覆盖。fence 完成值同时保护 resource、descriptor 范围和 command allocator 的回收。

本程序有独立实验选择与验证路径，公共实现集中在 advanced_labs.cpp。设备/API 检查、数值 compute 与图形渲染采用不同证据；不是每个程序都会弹出窗口。

## 构建和运行

```powershell
cmake --preset dx12
cmake --build --preset dx12-debug --target dx31_retire_resources --parallel 4
.\DirectX12\build\bin\Debug\dx31_retire_resources.exe
```

## 深入学习：数据、接口与执行

- [第 03 课：应用结构、RAII 与 GPU 使用期](../../chapters/03_lifetime/README.md)

### 本课的真实源码切片

来自 [DirectX12/common/advanced_labs.cpp](../../common/advanced_labs.cpp#L148)，第 148—191 行；这是现有实现的局部摘录，末尾可能处于函数中间，不是可单独编译的完整程序。

```cpp
void stagingCopy(DxDevice& c) {
    std::vector<uint32_t> original(257);
    for (uint32_t i = 0; i < 257; ++i)
        original[i] = i * 17 + 9;
    auto upload = c.buffer(257 * 4, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ);
    auto gpu = c.buffer(257 * 4, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_COPY_DEST);
    auto readback = c.buffer(257 * 4, D3D12_HEAP_TYPE_READBACK, D3D12_RESOURCE_STATE_COPY_DEST);
    writeBuffer(upload.Get(), original.data(), original.size() * 4);
    c.begin();
    c.cmd->CopyBufferRegion(gpu.Get(), 0, upload.Get(), 0, 257 * 4);
    auto b =
        transition(gpu.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COPY_SOURCE);
    c.cmd->ResourceBarrier(1, &b);
    c.cmd->CopyBufferRegion(readback.Get(), 0, gpu.Get(), 0, 257 * 4);
    c.submit();
    require(readBuffer(readback.Get(), 257) == original, "DEFAULT staging-copy result mismatch");
    std::cout << "Verified 257 DEFAULT round-trip words\n";
}
// 两组 allocator/list 与可变资源交替提交；等待的是本槽上次使用，不是每次提交后立即全局等待。
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
```

### 同主题跨 API 导航

| 路线 | 程序 |
| --- | --- |
| DirectX 12 | [dx31_retire_resources](README.md) |
| 对照后端 | [vk31_retire_resources](../../../Vulkan/native/lessons/vk31_retire_resources/README.md) |

### 先预测，再验证

将临时 upload 的回收点标到时间图上；指出“Execute/Submit 返回”之后还缺哪条证据。

参考推导：需要该资源最后一次使用所对应的 GPU 完成条件。提交返回仅说明工作已交给执行系统，不说明 copy 或 draw 已读完。

## 验证与边界

运行 `--smoke` 执行有限自动检查。数值实验读取实际 GPU 结果，图形实验检查非空图像并可查看读回。交互重建与性能还需额外证据；窗口 renderer 当前保留串行等待，双槽计算实验不会被描述为完整多帧呈现引擎。
