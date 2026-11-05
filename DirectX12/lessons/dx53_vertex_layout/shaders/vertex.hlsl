// 本章顶点阶段：核对输入属性位置/语义、矩阵内存布局以及最终裁剪空间位置。

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

// SV_InstanceID 只改变实例位置；顶点位置经 model 和 viewProjection 变到裁剪空间。
Pixel VertexMain(Vertex input, uint instance : SV_InstanceID) {
    Pixel p;
    float3 local = input.pos;
    if ((int)settings.x == 7)
        local += float3((int)(instance % 5) - 2, 0, -(int)(instance / 5)) * 1.5;
    float4 world = mul(model, float4(local, 1));
    p.world = world.xyz;
    p.normal = mul((float3x3)model, input.normal);
    p.uv = input.uv;
    p.pos = mul(viewProjection, world);
    return p;
}

// SV_VertexID 直接生成顶点，适合先学习 PSO 和绘制提交。
Pixel TriangleMain(uint id : SV_VertexID) {
    float2 points[3] = {float2(-.7, -.6), float2(.7, -.6), float2(0, .6)};
    float3 colors[3] = {float3(1, 0, 0), float3(0, 1, 0), float3(0, 0, 1)};
    Pixel p;
    p.pos = float4(points[id], 0, 1);
    p.normal = colors[id];
    p.uv = 0;
    p.world = p.pos.xyz;
    return p;
}
