
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

// 直接光 Cook-Torrance：GGX 分布、Schlick-GGX 遮蔽及 Schlick 菲涅耳。
// 与 Vulkan GLSL 使用相同场景和参数，方便逐项对照；此处未实现 IBL。
float3 PBR(float3 N, float3 V, float3 P, float metal, float r) {
    const float PI = 3.14159265359;
    float3 albedo = float3(.8, .25, .07), sum = .02 * albedo * (1 - metal),
           f0 = lerp(.04.xxx, albedo, metal);
    for (int i = 0; i < 4; ++i) {
        float3 lp = float3(i % 2 == 0 ? -6 : 6, i < 2 ? 6 : -4, 8);
        float3 delta = lp - P, L = normalize(delta), H = normalize(V + L);
        float NoV = max(dot(N, V), .001), NoL = max(dot(N, L), 0), NoH = max(dot(N, H), 0);
        float a = r * r, a2 = a * a, d = NoH * NoH * (a2 - 1) + 1,
              D = a2 / max(PI * d * d, .000001);
        float k = (r + 1) * (r + 1) / 8;
        float G = NoV / (NoV * (1 - k) + k) * NoL / (NoL * (1 - k) + k);
        float3 F = f0 + (1 - f0) * pow(1 - max(dot(V, H), 0), 5);
        sum += ((1 - F) * (1 - metal) * albedo / PI + D * G * F / max(4 * NoV * NoL, .001)) * 90 /
               dot(delta, delta) * NoL;
    }
    return sum;
}

// 区分调试颜色、纹理光照和 PBR 分支，最后执行 tone mapping/gamma。
float4 PixelMain(Pixel p) : SV_Target {
    int mode = (int)settings.x;
    float3 N = normalize(p.normal), c;
    if (mode <= 4)
        c = saturate(p.normal);
    else if (mode >= 9)
        c = PBR(N, normalize(float3(0, 0, 14) - p.world), p.world, settings.y, settings.z);
    else {
        c = mode >= 6 ? checker.Sample(texSampler, p.uv).rgb : float3(.25, .6, .9);
        c *= .15 + max(dot(N, normalize(float3(-3, 5, 4) - p.world)), 0);
    }
    if (mode >= 9)
        c = c / (1 + c);
    return float4(pow(max(c, 0), 1.0 / 2.2), 1);
}

// u0 是可写结构化缓冲；64 线程一组，4 组覆盖 CPU 端的 256 项。
RWStructuredBuffer<uint> values : register(u0);
[numthreads(64, 1, 1)] void ComputeMain(uint3 id : SV_DispatchThreadID) {
    if (id.x < 256)
        values[id.x] = id.x * id.x + 1;
}
