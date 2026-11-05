// 本章计算阶段：核对线程组尺寸、资源寄存器/绑定、边界检查和写后读依赖。
cbuffer Operation : register(b0) {
    uint mode;
    uint count;
    uint factor;
    uint bias;
};
cbuffer Parameters : register(b1) {
    uint4 parameters;
};
RWStructuredBuffer<uint> values : register(u0);
groupshared uint partial[64];
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
Texture2D<float4> mipSource : register(t0);
RWTexture2D<float4> mipOutput : register(u2);
