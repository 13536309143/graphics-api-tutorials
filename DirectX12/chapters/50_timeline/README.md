# 第 50 课 · Timeline、Fence 计数与生产消费链 · DirectX 12

[54 课目录](../../COURSE.md) · [对照精讲](../../../docs/compare/08_synchronization.md) · [上一课](../49_group_shared/README.md) · [下一课](../51_indirect/README.md)

## 本课目标与先修

学习目标：解释“Timeline、Fence 计数与生产消费链”解决的问题，找到实际对象和参数，运行对应实验并用数据/画面证明。先完成前一课，并能说明它交付的对象与完成条件；本课在该依赖上增加新契约。

## 为什么需要这一层

本章是官方 compute 后的工程扩展。用单调完成值表示多个进度点，比把所有工作压进一个“是否完成”的布尔状态更便于资源回收与多提交协调。数值必须对应真实 GPU 顺序。

## DirectX 12 的对象、参数与执行推导

ID3D12Fence 本来就是计数模型，queue Signal/Wait 建立生产消费，CPU 再等待完成值。相同队列链路用于验证语义；跨队列还需合法 resource state 与访问依赖，不声称此例展示独立引擎并行加速。

## 对照 Vulkan 原生：相同问题的另一种分工

设备额外查询并启用 timelineSemaphore。第一次提交 signal=1，第二次 wait=1/signal=2，host 等到 2 并读取 counter。使用 submit2 的阶段范围明确，不把 timeline semaphore 当作交换链 binary semaphore 的直接替代。

## 关键参数与可检查不变量

| 检查项 | Vulkan 表达 | DX12 表达 | 不变量 / 证据 |
| --- | --- | --- | --- |
| GPU 链 | signal1 → wait1/signal2 | queue Signal/Wait → 2 | 值对应真实执行进度 |
| CPU 等待 | WaitSemaphores(value=2) | fence/event 等 2 | 与 GPU 等待不是同一阻塞主体 |
| 范围边界 | 同队列语义实验 | 同队列语义实验 | 不作为独立硬件引擎并行证据 |

先为每项指出源码中的创建/绑定位置，以及证明它成立的实际日志、字节或图像。数值中的设备限制以查询为准；固定实验尺寸不当作所有设备的上限。

## nvpro_core2 框架对照

Context 特性需求需要包含所用 timeline 能力；Application 自身的帧调度与用户完成值应有明确边界。

这部分是原生 Vulkan 各课的框架对照，框架可运行入口仍为原有六课。如果本节需要新接入工作，会按正文说明；不把原生实验当作已完成的框架实现。

## 运行与实际检查

| 路线 | 实际实验 |
| --- | --- |
| DirectX 12 | [dx26_timeline_counter](../../lessons/dx26_timeline_counter/README.md) |
| Vulkan 原生 | [vk26_timeline_counter](../../../Vulkan/native/lessons/vk26_timeline_counter/README.md) |

从合集根目录执行；普通图形程序 Esc 退出，设备/计算/诊断程序输出结果后结束。从“开始学习.cmd”进入本页时，表格可直接点击运行。

```powershell
cmake --preset dx12
cmake --build --preset dx12-debug --target dx26_timeline_counter --parallel 4
.\DirectX12\build\bin\Debug\dx26_timeline_counter.exe
ctest --preset dx12-debug -R "^dx26_timeline_counter$" --output-on-failure
```

检查三个层次：创建/绑定/提交的接口约束；数据读回或目标画面；资源使用结束后的回收。诊断实验用合法信息或查询证明渠道可用，不故意保留 API 错误。图形 smoke 的像素检查与数值实验的逐项检查不同，具体实现见源码。

## 关键实现：从当前源码定位

来自 [DirectX12/common/advanced_labs.cpp](../../common/advanced_labs.cpp#L786)，第 786—829 行。下列是当前仓库的局部切片，可能在函数中间结束；完整上下文请打开文件，切片不作为单独可编译程序。

```cpp
        std::cout << "Verified queue producer 1 -> consumer 2 fence chain\n";
    } else if (id == 31) {
        for (int i = 0; i < 4; ++i)
            stagingCopy(c);
        std::cout << "Retired 4 UPLOAD/DEFAULT/READBACK generations after fence completion\n";
    } else if (id == 32) {
        ComPtr<ID3DBlob> cache, exactShader;
        {
            ComputeKernel first(c, options);
            first.execute(0, 257, 3, 7);
            require(readBuffer(first.readback.Get(), 257)[256] == 775,
                    "Initial cached PSO result mismatch");
            dxCheck(first.pipeline->GetCachedBlob(&cache));
            exactShader = first.compiledShader;
        }
        {
            ComputeKernel second(c, options, cache.Get(), exactShader.Get());
            second.execute(0, 257, 3, 7);
            require(readBuffer(second.readback.Get(), 257)[256] == 775,
                    "Reloaded cached PSO mismatch");
        }
        std::cout << "Verified cached PSO reload, bytes=" << cache->GetBufferSize() << '\n';
    } else if (id == 33) {
        dxCheck(c.queue->SetName(L"Course Graphics Queue"));
        dxCheck(c.cmd->SetName(L"Course Upload Commands"));
        dxCheck(c.allocator->SetName(L"Course Upload Allocator"));
        c.begin();
        c.submit();
        std::cout << "Named queue, command list and allocator for GPU debugging; event regions "
                     "require PIX helpers\n";
    } else {
        ComputeKernel kernel(c, options);
        uint32_t mode = id == 30 ? 2 : id == 14 || id == 15 ? 1 : 0;
        uint32_t count = id == 28 || id == 30 ? 257 : 128;
        for (uint32_t slot = 0; slot < (id == 15 ? 3u : 1u); ++slot) {
            uint32_t factor = id == 16 ? 9 : 3, bias = id == 16 ? 17 : 7;
            kernel.execute(mode, count, factor, bias, slot);
            auto result = readBuffer(kernel.readback.Get(), mode == 2 ? (count + 63) / 64 : count);
            if (mode == 2) {
                auto sum = std::accumulate(result.begin(), result.end(), 0u);
                require(sum == count * (count + 1) / 2, "Shared reduction result mismatch");
            } else {
                if (mode == 1) {
                    factor = slot == 0 ? 2 : slot == 1 ? 5 : 7;
```

阅读时按“前置对象 → 参数/布局 → 录制/调用 → 完成/可见性 → 释放”五步标注。共享 helper 减少重复代码，但不会取消每课的 API 契约。

## 受控修改实验

说明 host 等 counter=2 与 GPU queue 等 counter=1 的区别。

先记录预测，再只改一个因素；保留基线与修改后的输出。不要同时改变 shader、资源范围和同步，这样即使结果变好也无法归因。

### 参考推导与通过标准

前者阻塞 CPU 到某进度，后者让 GPU 工作在依赖成立后执行，CPU 可以继续录制其他工作。等待对象相同不意味着阻塞同一执行主体。

## 常见失败与排查顺序

CPU 提前 signal 一个并非真实 GPU 完成的值来“解锁”，会让后续生命周期证明失效。

先看第一条错误和当前执行路径，再看实际数据/绑定，最后看 shader 与画面。需要 GPU 证据时抓取正常交互帧；仅四帧的 smoke 适合自动检查，不适合手动抓帧。

## 自测与本课交付

能否不看源码回答：说明 host 等 counter=2 与 GPU queue 等 counter=1 的区别。

提交一份简短记录：输入与假设、唯一修改、预期、实际证据、原因、是否恢复基线。如果只能复述函数名而说不清何时允许访问/释放，先回到本课的数据流。

## 官方阅读位置与当前工程差异

[指定 Vulkan 教程对应位置](https://docs.vulkan.net.cn/tutorial/latest/11_Compute_Shader.html)。DX12 路线按同一概念顺序建立映射，其对象与约束以 Microsoft 文档和本后端代码为准。49—54 课包含在基础/compute 之后加入的工程扩展。

[版本与资料说明](../../../docs/REFERENCES.md) · [调试](../../../docs/DEBUGGING.md) · [功能完成度](../../../docs/FEATURE_MATRIX.md)
