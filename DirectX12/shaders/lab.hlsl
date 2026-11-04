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
