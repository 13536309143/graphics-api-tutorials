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
struct Vertex {
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
[numthreads(8, 8, 1)] void MipMain(uint3 id : SV_DispatchThreadID) {
    uint w, h;
    mipOutput.GetDimensions(w, h);
    if (id.x >= w || id.y >= h)
        return;
    uint sw, sh;
    mipSource.GetDimensions(sw, sh);
    uint2 base = id.xy * 2;
    float4 sum = 0;
    for (uint y = 0; y < 2; ++y)
        for (uint x = 0; x < 2; ++x)
            sum += mipSource.Load(int3(min(base + uint2(x, y), uint2(sw - 1, sh - 1)), 0));
    mipOutput[id.xy] = sum * .25;
}
