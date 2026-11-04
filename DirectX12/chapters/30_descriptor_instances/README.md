# 第 30 课 · 描述符池、Heap 与资源引用更新 · DirectX 12

[54 课目录](../../COURSE.md) · [对照精讲](../../../docs/compare/05_binding_constants.md) · [上一课](../29_descriptor_layout/README.md) · [下一课](../31_dynamic_offsets/README.md)

## 本课目标与先修

学习目标：解释“描述符池、Heap 与资源引用更新”解决的问题，找到实际对象和参数，运行对应实验并用数据/画面证明。先完成前一课，并能说明它交付的对象与完成条件；本课在该依赖上增加新契约。

## 为什么需要这一层

本实验先让输出指向资源 A，完成后改为 B，再证明 A 保持原值而 B 得到新结果。这样把资源本身、访问引用和更新时机三层拆开，而不是只检查一次创建是否返回成功。

## DirectX 12 的对象、参数与执行推导

计算实验用 root UAV 地址替换资源引用，纹理实验则通过 shader-visible heap 与 descriptor table 展示 heap 模型。二者不是同一接口，课程明确标注；CPU descriptor 的构建与 GPU handle 的绑定各有位置。

## 对照 Vulkan 原生：相同问题的另一种分工

pool 按描述符类型与数量提供分配容量，set 从 pool 分配。update 替换 binding 0 的 storage buffer，必须等原使用完成并遵循更新规则；普通 descriptor set 不能在 GPU 使用期间随意改写。

## 关键参数与可检查不变量

| 检查项 | Vulkan 表达 | DX12 表达 | 不变量 / 证据 |
| --- | --- | --- | --- |
| 替换顺序 | 完成 A 后更新 set 指 B | 完成 A 后 root UAV 指 B | 普通绑定不能在使用期间任意改写 |
| 完整证据 | A 不变 / B 得新结果 | A 不变 / B 得新结果 | 排除误写和读错资源 |
| 分配边界 | pool 类型/数量与 maxSets | heap 槽位与 root 机制 | 内存分配与 descriptor 分配分层 |

先为每项指出源码中的创建/绑定位置，以及证明它成立的实际日志、字节或图像。数值中的设备限制以查询为准；固定实验尺寸不当作所有设备的上限。

## nvpro_core2 框架对照

VMA 管理资源内存，不管理所有自定义 descriptor 更新。即使 helper 分配了 set，pool、set 和被引用资源仍有各自生命周期。

这部分是原生 Vulkan 各课的框架对照，框架可运行入口仍为原有六课。如果本节需要新接入工作，会按正文说明；不把原生实验当作已完成的框架实现。

## 运行与实际检查

| 路线 | 实际实验 |
| --- | --- |
| DirectX 12 | [dx45_descriptor_update](../../lessons/dx45_descriptor_update/README.md) |
| Vulkan 原生 | [vk45_descriptor_update](../../../Vulkan/native/lessons/vk45_descriptor_update/README.md) |

从合集根目录执行；普通图形程序 Esc 退出，设备/计算/诊断程序输出结果后结束。从“开始学习.cmd”进入本页时，表格可直接点击运行。

```powershell
cmake --preset dx12
cmake --build --preset dx12-debug --target dx45_descriptor_update --parallel 4
.\DirectX12\build\bin\Debug\dx45_descriptor_update.exe
ctest --preset dx12-debug -R "^dx45_descriptor_update$" --output-on-failure
```

检查三个层次：创建/绑定/提交的接口约束；数据读回或目标画面；资源使用结束后的回收。诊断实验用合法信息或查询证明渠道可用，不故意保留 API 错误。图形 smoke 的像素检查与数值实验的逐项检查不同，具体实现见源码。

## 关键实现：从当前源码定位

来自 [DirectX12/common/advanced_labs.cpp](../../common/advanced_labs.cpp#L685)，第 685—728 行。下列是当前仓库的局部切片，可能在函数中间结束；完整上下文请打开文件，切片不作为单独可编译程序。

```cpp
        c.cmd->Dispatch(2, 1, 1);
        auto b = transition(
            second.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);
        c.cmd->ResourceBarrier(1, &b);
        c.cmd->CopyBufferRegion(readback.Get(), 0, second.Get(), 0, 4096);
        c.submit();
        auto result = readBuffer(readback.Get(), 128);
        for (uint32_t i = 0; i < 128; ++i)
            require(result[i] == i * 5 + 11, "Root UAV replacement mismatch");
        require(readBuffer(kernel.readback.Get(), 128) == first,
                "Prior output was unexpectedly changed");
        std::cout << "Verified root descriptor replacement after prior GPU use completed\n";
    } else if (id == 46) {
        for (auto format :
             {DXGI_FORMAT_D32_FLOAT, DXGI_FORMAT_D24_UNORM_S8_UINT, DXGI_FORMAT_D16_UNORM}) {
            D3D12_FEATURE_DATA_FORMAT_SUPPORT q{format};
            dxCheck(c.device->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &q, sizeof(q)));
            std::cout << "depth format=" << format << ", support1=" << q.Support1 << '\n';
        }
    } else if (id == 47) {
        D3D12_RESOURCE_DESC desc{};
        desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        desc.Width = 257;
        desc.Height = 1;
        desc.DepthOrArraySize = 1;
        desc.MipLevels = 1;
        desc.SampleDesc.Count = 1;
        desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        auto allocation = c.device->GetResourceAllocationInfo(0, 1, &desc);
        require(allocation.SizeInBytes >= 257, "Allocation too small");
        auto upload = c.device->GetCustomHeapProperties(0, D3D12_HEAP_TYPE_UPLOAD);
        std::cout << "requested=257, allocation=" << allocation.SizeInBytes
                  << ", alignment=" << allocation.Alignment
                  << ", CPUPageProperty=" << upload.CPUPageProperty << '\n';
    } else if (id == 48) {
        auto initial = c.fence->GetCompletedValue();
        dxCheck(c.queue->Signal(c.fence.Get(), ++c.value));
        waitValue(c, c.value);
        require(c.fence->GetCompletedValue() > initial, "Fence counter failed to advance");
        dxCheck(c.queue->Signal(c.fence.Get(), ++c.value));
        waitValue(c, c.value);
        std::cout << "Verified monotonic fence reuse; DX12 fence is not reset like a binary Vulkan "
                     "fence\n";
    } else if (id == 51) {
```

阅读时按“前置对象 → 参数/布局 → 录制/调用 → 完成/可见性 → 释放”五步标注。共享 helper 减少重复代码，但不会取消每课的 API 契约。

## 受控修改实验

修改绑定到 B 后，为什么还要检查 A？

先记录预测，再只改一个因素；保留基线与修改后的输出。不要同时改变 shader、资源范围和同步，这样即使结果变好也无法归因。

### 参考推导与通过标准

这能发现 CPU 误写旧资源、错误绑定或读回来源混淆。只看 B 的某个值可能被旧内容偶然满足。

## 常见失败与排查顺序

把 descriptor update 当成数据 copy，或在旧提交仍读取同一描述符槽时覆盖它，会破坏接口与时间契约。

先看第一条错误和当前执行路径，再看实际数据/绑定，最后看 shader 与画面。需要 GPU 证据时抓取正常交互帧；仅四帧的 smoke 适合自动检查，不适合手动抓帧。

## 自测与本课交付

能否不看源码回答：修改绑定到 B 后，为什么还要检查 A？

提交一份简短记录：输入与假设、唯一修改、预期、实际证据、原因、是否恢复基线。如果只能复述函数名而说不清何时允许访问/释放，先回到本课的数据流。

## 官方阅读位置与当前工程差异

[指定 Vulkan 教程对应位置](https://docs.vulkan.net.cn/tutorial/latest/05_Uniform_buffers/01_Descriptor_pool_and_sets.html)。DX12 路线按同一概念顺序建立映射，其对象与约束以 Microsoft 文档和本后端代码为准。49—54 课包含在基础/compute 之后加入的工程扩展。

[版本与资料说明](../../../docs/REFERENCES.md) · [调试](../../../docs/DEBUGGING.md) · [功能完成度](../../../docs/FEATURE_MATRIX.md)
