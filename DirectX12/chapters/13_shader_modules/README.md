# 第 13 课 · 着色器编译、模块与反射 · DirectX 12

[54 课目录](../../COURSE.md) · [对照精讲](../../../docs/compare/02_triangle_pipeline.md) · [上一课](../12_pipeline_overview/README.md) · [下一课](../14_fixed_function/README.md)

## 本课目标与先修

学习目标：解释“着色器编译、模块与反射”解决的问题，找到实际对象和参数，运行对应实验并用数据/画面证明。先完成前一课，并能说明它交付的对象与完成条件；本课在该依赖上增加新契约。

## 为什么需要这一层

编译产物是 CPU 侧可传给 API 的代码表示，完整执行环境还需要管线与绑定。保存源码、编译参数、入口和目标版本，可以让一次 shader 错误被重现，而不是依赖 IDE 缓存。

## DirectX 12 的对象、参数与执行推导

D3DCompiler 生成 SM5.1 字节码；实验通过反射确认 ComputeMain 的工作组为 64×1×1 并检查资源绑定。Debug 产物可能带不同调试数据，管线缓存重建应复用完全一致的字节码，不能假定同源码重编译必然字节相同。 本例使用 SM5.x 的 ID3D11ShaderReflection 反射接口，这不表示渲染后端改成了 D3D11。

## 对照 Vulkan 原生：相同问题的另一种分工

glslc 生成 SPIR-V，模块创建要求长度与 word 对齐等合法条件。实验检查 magic、word 数和 module 创建。SPIR-V 的 ID bound 不是线程数量，模块也不决定最终 render target。

## 关键参数与可检查不变量

| 检查项 | Vulkan 表达 | DX12 表达 | 不变量 / 证据 |
| --- | --- | --- | --- |
| 产物表示 | SPIR-V uint32 words | DXBC / SM5.1 | 源码文本不是最终模块 |
| 组大小证据 | shader 声明/反汇编 | GetThreadGroupSize 反射 | host dispatch 与 shader 约定一致 |
| 缓存输入 | 模块字节与 pipeline 状态 | 完全相同的 DXBC 字节 | 同源码 Debug 重编译未必字节相同 |

先为每项指出源码中的创建/绑定位置，以及证明它成立的实际日志、字节或图像。数值中的设备限制以查询为准；固定实验尺寸不当作所有设备的上限。

## nvpro_core2 框架对照

使用项目固定 glslc 与路径，shader 与框架语言选择分开。最新官方 Slang 代码不能作为当前 GLSL 模块的直接输入。

这部分是原生 Vulkan 各课的框架对照，框架可运行入口仍为原有六课。如果本节需要新接入工作，会按正文说明；不把原生实验当作已完成的框架实现。

## 运行与实际检查

| 路线 | 实际实验 |
| --- | --- |
| DirectX 12 | [dx42_shader_inspection](../../lessons/dx42_shader_inspection/README.md) |
| Vulkan 原生 | [vk42_shader_inspection](../../../Vulkan/native/lessons/vk42_shader_inspection/README.md) |

从合集根目录执行；普通图形程序 Esc 退出，设备/计算/诊断程序输出结果后结束。从“开始学习.cmd”进入本页时，表格可直接点击运行。

```powershell
cmake --preset dx12
cmake --build --preset dx12-debug --target dx42_shader_inspection --parallel 4
.\DirectX12\build\bin\Debug\dx42_shader_inspection.exe
ctest --preset dx12-debug -R "^dx42_shader_inspection$" --output-on-failure
```

检查三个层次：创建/绑定/提交的接口约束；数据读回或目标画面；资源使用结束后的回收。诊断实验用合法信息或查询证明渠道可用，不故意保留 API 错误。图形 smoke 的像素检查与数值实验的逐项检查不同，具体实现见源码。

## 关键实现：从当前源码定位

来自 [DirectX12/common/advanced_labs.cpp](../../common/advanced_labs.cpp#L639)，第 639—682 行。下列是当前仓库的局部切片，可能在函数中间结束；完整上下文请打开文件，切片不作为单独可编译程序。

```cpp
    } else if (id == 39) {
        auto desc = c.queue->GetDesc();
        require(desc.Type == D3D12_COMMAND_LIST_TYPE_DIRECT, "Expected direct queue");
        std::cout << "queue type=" << desc.Type << ", priority=" << desc.Priority
                  << ", flags=" << desc.Flags << ", nodeMask=" << desc.NodeMask << '\n';
    } else if (id == 42) {
        auto shader = labShader(options, "ComputeMain");
        ComPtr<ID3D11ShaderReflection> reflection;
        dxCheck(D3DReflect(
            shader->GetBufferPointer(), shader->GetBufferSize(), IID_PPV_ARGS(&reflection)));
        UINT x{}, y{}, z{};
        reflection->GetThreadGroupSize(&x, &y, &z);
        require(x == 64 && y == 1 && z == 1, "Unexpected shader thread-group reflection");
        D3D11_SHADER_DESC desc{};
        dxCheck(reflection->GetDesc(&desc));
        std::cout << "Verified shader reflection: threads=" << x
                  << ", bindings=" << desc.BoundResources << '\n';
    } else if (id == 44) {
        auto upload = c.buffer(256, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ);
        auto readback = c.buffer(256, D3D12_HEAP_TYPE_READBACK, D3D12_RESOURCE_STATE_COPY_DEST);
        for (uint32_t generation = 0; generation < 4; ++generation) {
            std::vector<uint32_t> values(64, 41 + generation);
            writeBuffer(upload.Get(), values.data(), 256);
            c.begin();
            c.cmd->CopyBufferRegion(readback.Get(), 0, upload.Get(), 0, 256);
            c.submit();
            require(readBuffer(readback.Get(), 64) == values,
                    "Allocator reset contains stale commands");
        }
        std::cout << "Verified 4 allocator/list generations after completed fence values\n";
    } else if (id == 45) {
        ComputeKernel kernel(c, options);
        kernel.execute(0, 128, 2, 3);
        auto first = readBuffer(kernel.readback.Get(), 128);
        auto second = c.buffer(4096,
                               D3D12_HEAP_TYPE_DEFAULT,
                               D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                               D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
        auto readback = c.buffer(4096, D3D12_HEAP_TYPE_READBACK, D3D12_RESOURCE_STATE_COPY_DEST);
        c.begin();
        c.cmd->SetComputeRootSignature(kernel.signature.Get());
        c.cmd->SetPipelineState(kernel.pipeline.Get());
        uint32_t values[4] = {0, 128, 5, 11};
        c.cmd->SetComputeRoot32BitConstants(0, 4, values, 0);
```

阅读时按“前置对象 → 参数/布局 → 录制/调用 → 完成/可见性 → 释放”五步标注。共享 helper 减少重复代码，但不会取消每课的 API 契约。

## 受控修改实验

将 shader 的 local size 改为 32，但 host 的组数不改，预测覆盖范围和验证结果。

先记录预测，再只改一个因素；保留基线与修改后的输出。不要同时改变 shader、资源范围和同步，这样即使结果变好也无法归因。

### 参考推导与通过标准

总调用数量减半，尾部不会按原契约写入。反射/声明、dispatch 组数、缓冲容量和 CPU 检查必须共同更新。

## 常见失败与排查顺序

修改 exe 旁的副本会被下次 build 覆盖；应修改源目录，再检查复制与编译时间。

先看第一条错误和当前执行路径，再看实际数据/绑定，最后看 shader 与画面。需要 GPU 证据时抓取正常交互帧；仅四帧的 smoke 适合自动检查，不适合手动抓帧。

## 自测与本课交付

能否不看源码回答：将 shader 的 local size 改为 32，但 host 的组数不改，预测覆盖范围和验证结果。

提交一份简短记录：输入与假设、唯一修改、预期、实际证据、原因、是否恢复基线。如果只能复述函数名而说不清何时允许访问/释放，先回到本课的数据流。

## 官方阅读位置与当前工程差异

[指定 Vulkan 教程对应位置](https://docs.vulkan.net.cn/tutorial/latest/03_Drawing_a_triangle/02_Graphics_pipeline_basics/01_Shader_modules.html)。DX12 路线按同一概念顺序建立映射，其对象与约束以 Microsoft 文档和本后端代码为准。49—54 课包含在基础/compute 之后加入的工程扩展。

[版本与资料说明](../../../docs/REFERENCES.md) · [调试](../../../docs/DEBUGGING.md) · [功能完成度](../../../docs/FEATURE_MATRIX.md)
