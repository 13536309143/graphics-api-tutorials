# 第 35 课 · 图像布局、资源状态与读写依赖 · DirectX 12

[54 课目录](../../COURSE.md) · [对照精讲](../../../docs/compare/08_synchronization.md) · [上一课](../34_image_upload/README.md) · [下一课](../36_samplers/README.md)

## 本课目标与先修

学习目标：解释“图像布局、资源状态与读写依赖”解决的问题，找到实际对象和参数，运行对应实验并用数据/画面证明。先完成前一课，并能说明它交付的对象与完成条件；本课在该依赖上增加新契约。

## 为什么需要这一层

为每次访问写下生产者与消费者：谁写、谁读、在哪个队列、读写范围是什么。布局/状态表示合法用途的一部分，执行顺序与可见性还要通过同步规则建立。

## DirectX 12 的对象、参数与执行推导

legacy transition barrier 表达 COPY_DEST、UAV、shader resource 等用途转换。compute mip 每级有自己的 subresource state；UAV 写后下级采样的依赖由对应转换建立。跨队列还需 fence Signal/Wait。

## 对照 Vulkan 原生：相同问题的另一种分工

copy-write 到 fragment-sampled-read 需要对应 stage/access 和布局转换。mip 生成逐级把前一级从 transfer-dst 切成 transfer-src，最终各级转 shader-read；不能仅在整个流程结束后随意改一个枚举。

## 关键参数与可检查不变量

| 检查项 | Vulkan 表达 | DX12 表达 | 不变量 / 证据 |
| --- | --- | --- | --- |
| 生产者 | COPY/BLIT write | COPY_DEST 或 UAV write | 写入来源决定依赖 |
| 消费者 | fragment/compute sampled read | pixel/non-pixel shader read | 按真实使用者选择范围 |
| 每级状态 | 逐 mip layout/access | 逐 subresource state | 不假定全纹理始终同一状态 |

先为每项指出源码中的创建/绑定位置，以及证明它成立的实际日志、字节或图像。数值中的设备限制以查询为准；固定实验尺寸不当作所有设备的上限。

## nvpro_core2 框架对照

GENERAL→GENERAL 也可能需要依赖。RenderTarget 与 ImGui 的颜色写后采样正是布局不变而访问变化的例子。

这部分是原生 Vulkan 各课的框架对照，框架可运行入口仍为原有六课。如果本节需要新接入工作，会按正文说明；不把原生实验当作已完成的框架实现。

## 运行与实际检查

| 路线 | 实际实验 |
| --- | --- |
| DirectX 12 | [dx13_staging_copy](../../lessons/dx13_staging_copy/README.md) |
| DirectX 12 | [dx18_mip_chain](../../lessons/dx18_mip_chain/README.md) |
| Vulkan 原生 | [vk13_staging_copy](../../../Vulkan/native/lessons/vk13_staging_copy/README.md) |
| Vulkan 原生 | [vk18_mip_chain](../../../Vulkan/native/lessons/vk18_mip_chain/README.md) |

从合集根目录执行；普通图形程序 Esc 退出，设备/计算/诊断程序输出结果后结束。从“开始学习.cmd”进入本页时，表格可直接点击运行。

```powershell
cmake --preset dx12
cmake --build --preset dx12-debug --target dx13_staging_copy --parallel 4
.\DirectX12\build\bin\Debug\dx13_staging_copy.exe
ctest --preset dx12-debug -R "^dx13_staging_copy$" --output-on-failure
```

检查三个层次：创建/绑定/提交的接口约束；数据读回或目标画面；资源使用结束后的回收。诊断实验用合法信息或查询证明渠道可用，不故意保留 API 错误。图形 smoke 的像素检查与数值实验的逐项检查不同，具体实现见源码。

## 关键实现：从当前源码定位

来自 [DirectX12/common/advanced_labs.cpp](../../common/advanced_labs.cpp#L148)，第 148—191 行。下列是当前仓库的局部切片，可能在函数中间结束；完整上下文请打开文件，切片不作为单独可编译程序。

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

阅读时按“前置对象 → 参数/布局 → 录制/调用 → 完成/可见性 → 释放”五步标注。共享 helper 减少重复代码，但不会取消每课的 API 契约。

## 受控修改实验

把纹理消费者从 pixel/fragment 改为 compute，哪些同步描述也要一起改变？

先记录预测，再只改一个因素；保留基线与修改后的输出。不要同时改变 shader、资源范围和同步，这样即使结果变好也无法归因。

### 参考推导与通过标准

消费者阶段/访问或 state 应反映 compute 读取。直接复用只针对片元的依赖不一定覆盖新的使用者。

## 常见失败与排查顺序

coherent、Map 成功、descriptor 中的 layout 或 CPU 提交顺序，任何一个都不能独自证明完整 GPU 写后读关系。

先看第一条错误和当前执行路径，再看实际数据/绑定，最后看 shader 与画面。需要 GPU 证据时抓取正常交互帧；仅四帧的 smoke 适合自动检查，不适合手动抓帧。

## 自测与本课交付

能否不看源码回答：把纹理消费者从 pixel/fragment 改为 compute，哪些同步描述也要一起改变？

提交一份简短记录：输入与假设、唯一修改、预期、实际证据、原因、是否恢复基线。如果只能复述函数名而说不清何时允许访问/释放，先回到本课的数据流。

## 官方阅读位置与当前工程差异

[指定 Vulkan 教程对应位置](https://docs.vulkan.net.cn/tutorial/latest/06_Texture_mapping/00_Images.html)。DX12 路线按同一概念顺序建立映射，其对象与约束以 Microsoft 文档和本后端代码为准。49—54 课包含在基础/compute 之后加入的工程扩展。

[版本与资料说明](../../../docs/REFERENCES.md) · [调试](../../../docs/DEBUGGING.md) · [功能完成度](../../../docs/FEATURE_MATRIX.md)
