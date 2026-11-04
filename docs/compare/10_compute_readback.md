# 10 · 计算着色器、Storage Buffer、UAV 与读回

[对照路线](../COMPARE.md) · [交互式对照](index.html#10_compute_readback) · [上一章](09_commands_frames.md) · [下一章](11_models_animation.md)

计算课去掉窗口和光栅化，只验证 GPU 按指定规则处理数据。本仓库用 `result[i] = i*i + 1` 处理 256 个整数，逐项验证比“打印第一个值”更能发现分组或索引错误。

## 把工作范围算清楚

Vulkan GLSL 使用 local_size_x=64；HLSL 使用 numthreads(64,1,1)。host dispatch 4 组，所以总共发出 256 个线程。shader 使用全局调用索引，不是仅组内索引；使用组内 0..63 会让四组反复写同一段。

任意元素数 N 的组数通常向上取整为 `(N + localSize - 1) / localSize`，shader 还应判断 `i < N`。这解决覆盖范围，不解决组间同步。一个 dispatch 内不同工作组不能随意依赖对方恰好先执行；跨组生产/消费通常需要重新组织算法或拆分 dispatch 并建立依赖。

| 概念 | Vulkan / nvpro | DX12 | OpenGL 路线 |
| --- | --- | --- | --- |
| 可写缓冲接口 | storage-buffer descriptor | UAV / RWStructuredBuffer | 当前 3.3 基线没有对应 compute 课程 |
| 执行管线 | compute pipeline | compute PSO | 若扩展需提高版本或查询扩展 |
| 全局线程索引 | gl_GlobalInvocationID | SV_DispatchThreadID | 更高基线可用相应 GLSL 内建 |
| CPU 读取 | 可见内存 + 依赖 + fence | copy 到 READBACK + fence | 不能用现有粒子课冒充 compute |

OpenGL 32 的粒子使用实例化绘制，并非本仓库已经有 compute 后端。对比应把“API 版本支持能力”和“课程是否实现”分别写清楚。

## Vulkan 与 DX12 读回为什么不同

Vulkan 当前计算缓冲选择 host-visible coherent 内存，GPU 直接写它。完成计算后录制 compute-storage-write 到 host-read 的内存依赖；immediate 提交等待 fence，再 map 并核对。若改成非 coherent 内存，还要按规则 invalidate。

DX12 的计算数据在 DEFAULT heap，供 UAV 写入；之后把资源转为 COPY_SOURCE，复制到 READBACK heap，等待 GPU，CPU 再 map。额外拷贝是内存布局选择的一部分，不能只把 Map 函数名对应起来比较行数。

## 实验：从 256 改到 257

必须一起改容量、组数、边界和验证循环。正确组数是 5，最后一组只有一个有效结果。若只把 CPU 检查循环改为 257，会读超出分配；若只改 buffer 大小却仍 dispatch 4 组，最后一项不会被写。

第二个实验把公式改为 `3*i+7`，分别修改 shader 和 CPU 参考函数。先故意只修改一端，确认验证确实失败；再对齐两端确认通过。这样检验的是检查工具是否能识别错误，而不只是程序能否退出。

## 从计算到图形的下一步

让计算生成实例位置，再由顶点阶段读取。依赖应从 compute write 指向真正消费者的访问类型，例如 vertex input 或 shader storage read；不要机械复用 host-read 屏障。GPU 直接消费时不必为了“同步”先把结果读回 CPU。

验收记录组大小、组数量、元素数量、缓冲字节数、生产者/消费者与完成条件。控制台课成功后应查看“运行输出”，没有弹出窗口不是异常。

<!-- generated-source-comparison -->

## 与本仓库实现并排核对

以下切片直接取自当前仓库，省略了前后文。行号用于定位，不能把片段当作完整可编译程序。

| 路线 | 可运行参照 / 范围 |
| --- | --- |
| OpenGL | 基线外 |
| Vulkan 原生 | [vk08_compute_readback](../../Vulkan/native/lessons/vk08_compute_readback/README.md) |
| Vulkan / nvpro_core2 | [nv05_compute_readback](../../Vulkan/nvpro/lessons/nv05_compute_readback/README.md) |
| DirectX 12 | [dx08_compute_uav](../../DirectX12/lessons/dx08_compute_uav/README.md) |

### OpenGL · 基线外

当前 OpenGL 3.3 路线没有 compute 实现；粒子实例化不能作为 compute 对照。


### Vulkan 原生 · 已实现

全局调用索引覆盖 256 项，host 逐项核对，不仅检查第一个值。

[Vulkan/common/shaders/square.comp](../../Vulkan/common/shaders/square.comp#L2)，第 2—12 行：

```glsl
layout(local_size_x = 64) in;
layout(binding = 0) buffer Values {
    uint values[];
};

// 每工作组 64 个线程；host dispatch 4 组，结果通过 storage buffer 读回。
void main() {
    uint i = gl_GlobalInvocationID.x;
    if (i < 256)
        values[i] = i * i + 1;
}
```

### Vulkan / nvpro_core2 · 已实现

Context 提供队列和设备；计算管线与资源依赖仍是 Vulkan。

[Vulkan/common/scene_vk.hpp](../../Vulkan/common/scene_vk.hpp#L442)，第 442—478 行：

```cpp
inline void computeSquares(const VkContextRef& c, const std::filesystem::path& shaders) {
    VkBufferResource storage;
    storage.init(c, 256 * 4, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
    VkDescriptorSetLayoutBinding binding{
        0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr};
    VkDescriptorSetLayoutCreateInfo li{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    li.bindingCount = 1;
    li.pBindings = &binding;
    VkDescriptorSetLayout layout;
    vkCheck(vkCreateDescriptorSetLayout(c.device, &li, nullptr, &layout));
    VkPipelineLayoutCreateInfo pl{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    pl.setLayoutCount = 1;
    pl.pSetLayouts = &layout;
    VkPipelineLayout pipelineLayout;
    vkCheck(vkCreatePipelineLayout(c.device, &pl, nullptr, &pipelineLayout));
    VkDescriptorPoolSize size{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1};
    VkDescriptorPoolCreateInfo pool{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    pool.maxSets = 1;
    pool.poolSizeCount = 1;
    pool.pPoolSizes = &size;
    VkDescriptorPool descriptorPool;
    vkCheck(vkCreateDescriptorPool(c.device, &pool, nullptr, &descriptorPool));
    VkDescriptorSetAllocateInfo ai{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    ai.descriptorPool = descriptorPool;
    ai.descriptorSetCount = 1;
    ai.pSetLayouts = &layout;
    VkDescriptorSet set;
    vkCheck(vkAllocateDescriptorSets(c.device, &ai, &set));
    VkDescriptorBufferInfo bi{storage.buffer, 0, 256 * 4};
    VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    write.dstSet = set;
    write.descriptorCount = 1;
    write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    write.pBufferInfo = &bi;
    vkUpdateDescriptorSets(c.device, 1, &write, 0, nullptr);
    VkShaderModule cs = shaderModule(c.device, shaders / "square.comp.spv");
    VkComputePipelineCreateInfo ci{VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};
```

### DirectX 12 · 已实现

DEFAULT UAV 结果拷贝到 READBACK，经 fence 后 Map。

[DirectX12/common/dx12_course.hpp](../../DirectX12/common/dx12_course.hpp#L195)，第 195—242 行：

```cpp
inline void dxCompute(DxDevice& c) {
    auto storage = c.buffer(1024,
                            D3D12_HEAP_TYPE_DEFAULT,
                            D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                            D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS),
         readback = c.buffer(1024, D3D12_HEAP_TYPE_READBACK, D3D12_RESOURCE_STATE_COPY_DEST);
    D3D12_ROOT_PARAMETER root{};
    root.ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV;
    root.Descriptor.ShaderRegister = 0;
    D3D12_ROOT_SIGNATURE_DESC desc{};
    desc.NumParameters = 1;
    desc.pParameters = &root;
    auto signature = rootSignature(c, desc);
    auto cs = compileHlsl("ComputeMain", "cs_5_1");
    D3D12_COMPUTE_PIPELINE_STATE_DESC pso{};
    pso.pRootSignature = signature.Get();
    pso.CS = {cs->GetBufferPointer(), cs->GetBufferSize()};
    ComPtr<ID3D12PipelineState> pipeline;
    dxCheck(c.device->CreateComputePipelineState(&pso, IID_PPV_ARGS(&pipeline)));
    c.begin();
    c.cmd->SetComputeRootSignature(signature.Get());
    c.cmd->SetPipelineState(pipeline.Get());
    c.cmd->SetComputeRootUnorderedAccessView(0, storage->GetGPUVirtualAddress());
    c.cmd->Dispatch(4, 1, 1);
    auto b = transition(
        storage.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);
    c.cmd->ResourceBarrier(1, &b);
    c.cmd->CopyBufferRegion(readback.Get(), 0, storage.Get(), 0, 1024);
    c.submit();
    void* mapped;
    D3D12_RANGE readRange{0, 1024};
    dxCheck(readback->Map(0, &readRange, &mapped));
    for (uint32_t i = 0; i < 256; ++i)
        if (static_cast<uint32_t*>(mapped)[i] != i * i + 1)
            throw std::runtime_error("GPU compute mismatch");
    D3D12_RANGE written{0, 0};
    readback->Unmap(0, &written);
    std::cout << "Verified all 256 GPU compute results\n";
    c.checkMessages();
}
class DxScene {
public:
    DxDevice& c;
    int mode;
    Geometry mesh;
    ComPtr<ID3D12Resource> vertices, indices, uniform, texture;
    ComPtr<ID3D12DescriptorHeap> srvHeap;
    ComPtr<ID3D12RootSignature> signature;
```
