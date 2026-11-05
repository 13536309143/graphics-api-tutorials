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
