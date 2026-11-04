# dx30_shared_reduce · 组共享内存归约

[54 课目录](../../COURSE.md)

## 目的与实际执行

groupshared 与 GroupMemoryBarrierWithGroupSync 承担相同算法职责。线程在屏障前非一致 return 可能造成未定义结果或卡住，因此边界检查只改变贡献值，不让部分 lane 跳过同步。

本程序有独立实验选择与验证路径，公共实现集中在 advanced_labs.cpp。设备/API 检查、数值 compute 与图形渲染采用不同证据；不是每个程序都会弹出窗口。

## 构建和运行

```powershell
cmake --preset dx12
cmake --build --preset dx12-debug --target dx30_shared_reduce --parallel 4
.\DirectX12\build\bin\Debug\dx30_shared_reduce.exe
```

## 深入学习：数据、接口与执行

- [第 49 课：组共享内存、归约与屏障一致性](../../chapters/49_group_shared/README.md)

### 本课的真实源码切片

来自 [DirectX12/shaders/lab.hlsl](../../shaders/lab.hlsl#L12)，第 12—55 行；这是现有实现的局部摘录，末尾可能处于函数中间，不是可单独编译的完整程序。

```hlsl
[numthreads(64, 1, 1)] void ComputeMain(uint3 global : SV_DispatchThreadID,
                                        uint3 local : SV_GroupThreadID,
                                        uint3 group : SV_GroupID) {
    uint i = global.x, lane = local.x;
    if (mode == 2) {
        partial[lane] = i < count ? i + 1 : 0;
        GroupMemoryBarrierWithGroupSync();
        for (uint stride = 32; stride > 0; stride >>= 1) {
            if (lane < stride)
                partial[lane] += partial[lane + stride];
            GroupMemoryBarrierWithGroupSync();
        }
        if (lane == 0)
            values[group.x] = partial[0];
    } else if (i < count) {
        values[i] = i * (mode == 1 ? parameters.x : factor) + (mode == 1 ? parameters.y : bias);
    }
} struct Vertex {
    float3 position;
    float3 normal;
    float2 uv;
};
RWStructuredBuffer<Vertex> vertices : register(u1);
cbuffer Tick : register(b2) {
    float time;
    uint particleCount;
};
[numthreads(64, 1, 1)] void ParticleMain(uint3 id : SV_DispatchThreadID) {
    uint i = id.x;
    if (i >= particleCount * 3)
        return;
    uint particle = i / 3, corner = i % 3;
    float phase = particle * 2.39996323 + time * .35;
    float radius = .1 + .75 * sqrt((float)particle / particleCount);
    float2 center = float2(cos(phase), sin(phase)) * radius;
    float a = corner * 2.0943951;
    Vertex v;
    v.position = float3(center + float2(cos(a), sin(a)) * .028, 0);
    v.normal = float3(.25 + .7 * (particle % 7) / 6, .65, .95);
    v.uv = 0;
    vertices[i] = v;
} Texture2D<float4> mipSource : register(t0);
RWTexture2D<float4> mipOutput : register(u2);
[numthreads(8, 8, 1)] void MipMain(uint3 id : SV_DispatchThreadID) {
```

### 同主题跨 API 导航

| 路线 | 程序 |
| --- | --- |
| DirectX 12 | [dx30_shared_reduce](README.md) |
| 对照后端 | [vk30_shared_reduce](../../../Vulkan/native/lessons/vk30_shared_reduce/README.md) |

### 先预测，再验证

若尾部 lane 在第一个 barrier 前直接 return，为什么不是一种安全优化？

参考推导：组中部分线程仍在等待完整同步，屏障的控制流条件不再成立。应让它们贡献零并继续参与屏障。

## 验证与边界

运行 `--smoke` 执行有限自动检查。数值实验读取实际 GPU 结果，图形实验检查非空图像并可查看读回。交互重建与性能还需额外证据；窗口 renderer 当前保留串行等待，双槽计算实验不会被描述为完整多帧呈现引擎。
