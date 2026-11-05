// 本章计算阶段：核对线程组尺寸、资源寄存器/绑定、边界检查和写后读依赖。

// b0 保存每帧相机矩阵，b1 对应 CPU Draw；column_major 匹配 GLM 默认存储。
cbuffer Frame : register(b0) {
    column_major float4x4 viewProjection;
};
cbuffer Draw : register(b1) {
    column_major float4x4 model;
    float4 settings;
};
Texture2D<float4> checker : register(t0);
SamplerState texSampler : register(s0);
struct Vertex {
    float3 pos : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD0;
};
struct Pixel {
    float4 pos : SV_Position;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD0;
    float3 world : TEXCOORD1;
};

// u0 是可写结构化缓冲；64 线程一组，4 组覆盖 CPU 端的 256 项。
RWStructuredBuffer<uint> values : register(u0);
[numthreads(64, 1, 1)] void ComputeMain(uint3 id : SV_DispatchThreadID) {
    if (id.x < 256)
        values[id.x] = id.x * id.x + 1;
}
