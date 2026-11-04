# 第 53 课 · GPU 时间戳、管线缓存与测量边界 · DirectX 12

[54 课目录](../../COURSE.md) · [对照精讲](../../../docs/compare/14_debug_performance.md) · [上一课](../52_camera_material/README.md) · [下一课](../54_capstone/README.md)

## 本课目标与先修

学习目标：解释“GPU 时间戳、管线缓存与测量边界”解决的问题，找到实际对象和参数，运行对应实验并用数据/画面证明。先完成前一课，并能说明它交付的对象与完成条件；本课在该依赖上增加新契约。

## 为什么需要这一层

这是工程扩展。时间戳观察 GPU 某段工作，CPU 调用耗时观察录制/提交；二者不能互相替代。缓存实验验证序列化和重建后结果一致，不以一次运行就宣称某 API 更快。

## DirectX 12 的对象、参数与执行推导

queue timestamp frequency 将 query ticks 转成时间，结果经 ResolveQueryData/readback 等待后读取。缓存 PSO 重建复用完全相同的编译字节码；Debug 重编译可能引入变化，不能仅凭源码相同断言 cache 一定兼容。

## 对照 Vulkan 原生：相同问题的另一种分工

timestampValidBits 与 timestampPeriod 决定计时解释；本例记录填充操作前后时间并处理有效位。pipeline cache 在同一设备/驱动条件下保存并重建 compute pipeline，实际执行后再核对数值。

## 关键参数与可检查不变量

| 检查项 | Vulkan 表达 | DX12 表达 | 不变量 / 证据 |
| --- | --- | --- | --- |
| 计时单位 | 有效 bits + period | ticks / queue frequency | 分别解释 GPU 与 CPU 耗时 |
| 缓存输入 | 相容 pipeline 与 cache bytes | 相同 DXBC 与 PSO 描述 | 不是所有版本/驱动可通用数据 |
| 结论范围 | 单操作/重建验证 | 单操作/重建验证 | 不是跨 API benchmark |

先为每项指出源码中的创建/绑定位置，以及证明它成立的实际日志、字节或图像。数值中的设备限制以查询为准；固定实验尺寸不当作所有设备的上限。

## nvpro_core2 框架对照

UI、离屏与框架调度应从场景 pass 测量中区分。工具抓帧的额外开销也不等于发布版正常帧成本。

这部分是原生 Vulkan 各课的框架对照，框架可运行入口仍为原有六课。如果本节需要新接入工作，会按正文说明；不把原生实验当作已完成的框架实现。

## 运行与实际检查

| 路线 | 实际实验 |
| --- | --- |
| DirectX 12 | [dx25_timestamps](../../lessons/dx25_timestamps/README.md) |
| DirectX 12 | [dx32_pipeline_cache](../../lessons/dx32_pipeline_cache/README.md) |
| Vulkan 原生 | [vk25_timestamps](../../../Vulkan/native/lessons/vk25_timestamps/README.md) |
| Vulkan 原生 | [vk32_pipeline_cache](../../../Vulkan/native/lessons/vk32_pipeline_cache/README.md) |

从合集根目录执行；普通图形程序 Esc 退出，设备/计算/诊断程序输出结果后结束。从“开始学习.cmd”进入本页时，表格可直接点击运行。

```powershell
cmake --preset dx12
cmake --build --preset dx12-debug --target dx25_timestamps --parallel 4
.\DirectX12\build\bin\Debug\dx25_timestamps.exe
ctest --preset dx12-debug -R "^dx25_timestamps$" --output-on-failure
```

检查三个层次：创建/绑定/提交的接口约束；数据读回或目标画面；资源使用结束后的回收。诊断实验用合法信息或查询证明渠道可用，不故意保留 API 错误。图形 smoke 的像素检查与数值实验的逐项检查不同，具体实现见源码。

## 关键实现：从当前源码定位

来自 [DirectX12/common/advanced_labs.cpp](../../common/advanced_labs.cpp#L210)，第 210—253 行。下列是当前仓库的局部切片，可能在函数中间结束；完整上下文请打开文件，切片不作为单独可编译程序。

```cpp
void timestamps(DxDevice& c) {
    D3D12_QUERY_HEAP_DESC desc{};
    desc.Type = D3D12_QUERY_HEAP_TYPE_TIMESTAMP;
    desc.Count = 2;
    ComPtr<ID3D12QueryHeap> heap;
    dxCheck(c.device->CreateQueryHeap(&desc, IID_PPV_ARGS(&heap)));
    auto upload = c.buffer(1024 * 1024, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ);
    auto gpu = c.buffer(1024 * 1024, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_COPY_DEST);
    auto result = c.buffer(16, D3D12_HEAP_TYPE_READBACK, D3D12_RESOURCE_STATE_COPY_DEST);
    std::vector<uint8_t> bytes(1024 * 1024);
    writeBuffer(upload.Get(), bytes.data(), bytes.size());
    c.begin();
    c.cmd->EndQuery(heap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, 0);
    c.cmd->CopyBufferRegion(gpu.Get(), 0, upload.Get(), 0, bytes.size());
    c.cmd->EndQuery(heap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, 1);
    c.cmd->ResolveQueryData(heap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, 0, 2, result.Get(), 0);
    c.submit();
    auto words = readBuffer(result.Get(), 4);
    uint64_t ticks[2]{};
    std::memcpy(ticks, words.data(), 16);
    UINT64 frequency{};
    dxCheck(c.queue->GetTimestampFrequency(&frequency));
    require(frequency != 0, "Invalid timestamp frequency");
    std::cout << "GPU copy measurement=" << double(ticks[1] - ticks[0]) * 1000 / frequency
              << " ms (not a cross-API benchmark)\n";
}
// 线性 UNORM mip 在 GPU 上由 SRV→UAV 逐级生成；sRGB 材质链用线性平均后再编码的 CPU 路径。
struct GpuTexture {
    DxDevice& c;
    ComPtr<ID3D12Resource> texture;
    std::vector<Pixels> reference;
    DXGI_FORMAT viewFormat;
    GpuTexture(
        DxDevice& context, const LabOptions& options, Pixels pixels, bool mips, bool srgb = false)
        : c(context) {
        reference = mips ? mipReference(pixels, srgb) : std::vector<Pixels>{pixels};
        const UINT levels = UINT(reference.size());
        const bool gpuMips = mips && !srgb;
        viewFormat = srgb ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM;
        D3D12_RESOURCE_DESC desc{};
        desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        desc.Width = pixels.width;
        desc.Height = pixels.height;
        desc.DepthOrArraySize = 1;
```

阅读时按“前置对象 → 参数/布局 → 录制/调用 → 完成/可见性 → 释放”五步标注。共享 helper 减少重复代码，但不会取消每课的 API 契约。

## 受控修改实验

列出一个公平跨 API 性能实验至少需要固定的六项条件。

先记录预测，再只改一个因素；保留基线与修改后的输出。不要同时改变 shader、资源范围和同步，这样即使结果变好也无法归因。

### 参考推导与通过标准

工作量、分辨率、shader/材质、场景/相机、采样/呈现策略、构建配置；还应预热、重复采样并报告 CPU/GPU/整帧时间及波动。

## 常见失败与排查顺序

把 Debug smoke 的总秒数、垂直同步限制的 FPS 或一次最小值当成 API 性能排名，均缺少可比前提。

先看第一条错误和当前执行路径，再看实际数据/绑定，最后看 shader 与画面。需要 GPU 证据时抓取正常交互帧；仅四帧的 smoke 适合自动检查，不适合手动抓帧。

## 自测与本课交付

能否不看源码回答：列出一个公平跨 API 性能实验至少需要固定的六项条件。

提交一份简短记录：输入与假设、唯一修改、预期、实际证据、原因、是否恢复基线。如果只能复述函数名而说不清何时允许访问/释放，先回到本课的数据流。

## 官方阅读位置与当前工程差异

[指定 Vulkan 教程对应位置](https://docs.vulkan.net.cn/tutorial/latest/11_Compute_Shader.html)。DX12 路线按同一概念顺序建立映射，其对象与约束以 Microsoft 文档和本后端代码为准。49—54 课包含在基础/compute 之后加入的工程扩展。

[版本与资料说明](../../../docs/REFERENCES.md) · [调试](../../../docs/DEBUGGING.md) · [功能完成度](../../../docs/FEATURE_MATRIX.md)
