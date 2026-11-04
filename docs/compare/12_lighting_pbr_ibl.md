# 12 · 光照、PBR、IBL 与跨 API 画面对齐

[对照路线](../COMPARE.md) · [交互式对照](index.html#12_lighting_pbr_ibl) · [上一章](11_models_animation.md) · [下一章](13_multipass_effects.md)

光照公式主要属于材质和图形学层。三种 API 可以执行同一 BRDF，但仍可能因为坐标、贴图格式、采样或输出编码不同而产生不同画面。先统一这些输入，再讨论公式或驱动差异。

## 从 Phong 到微表面

OpenGL 09/10 的 Phong 把环境、漫反射和镜面高光组合起来，容易观察法线、光线和视线的作用。PBR 23 用金属度/粗糙度表达材质，并采用微表面模型。这里的“PBR”不是保证物理完全精确，而是采用相对一致的能量分配与反射参数。

对一束直接光，镜面项的结构是 `D * G * F / max(4 * NoV * NoL, epsilon)`。D 描述微表面法线分布；G 描述几何遮蔽；F 描述菲涅耳反射。diffuse 则按 `(1-F)*(1-metallic)` 分配，再乘 albedo/PI。最终还要乘光源辐射、NoL 及距离衰减。

粗糙度影响高光形状，不是简单把最终颜色乘一个暗度系数。金属的镜面颜色趋向 albedo，普通介质的 F0 在此教学模型中约为 0.04。数值下限防止接近掠射角或低粗糙度时除零；它不能代替合法输入范围与法线归一化。

## 本仓库各路线的范围

| 内容 | OpenGL | Vulkan 原生 / nvpro | DX12 |
| --- | --- | --- | --- |
| 直接光球阵列 | 23/24 | vk09 / nv06 | dx09 |
| PBR 贴图 | 24/模型材质 | 未完整迁移 | 未完整迁移 |
| IBL 预计算 | 25、Environment | 未完整迁移 | 未完整迁移 |
| HDR/Bloom 合成 | 21/22 | 未完整迁移 | 未完整迁移 |

nv06 和 vk09 复用 SceneVk 的 shader，区别主要是上下文、目标和 UI。DX12 HLSL 与 Vulkan GLSL 的直接光球阵列采用相同共享几何和参数；OpenGL 原课程的场景、光源和 IBL 配置不同。四张图可用于理解效果，不能当作逐像素一致的后端回归基准。

## IBL 为什么有三种预计算

OpenGL Environment 先把 HDR 经纬图转为 cubemap。漫反射半球积分输出 irradiance；不同粗糙度的镜面采样输出 prefilter 的 mip 链；BRDF LUT 把视角和粗糙度相关的一部分积分存成二维纹理。运行时将它们与材质参数组合，避免每像素重新采样大量环境方向。

prefilter 的 mip 在这里不只是“远处更模糊”的普通缩小层，而对应不同材质粗糙度。使用 `roughness * 4` 是因为当前示例有五级；改级数时 shader、sampler LOD 与资源创建要一起调整。AO 主要作用于间接光的简化遮挡，不应随意把所有直接光也一律乘黑。

## 做一次公平的画面对齐实验

固定相机、模型、时间、光源位置/强度、albedo、metallic、roughness、背景色和曝光。关闭 IBL 与纹理，先比较一个球的法线调试输出，再比较 NoL，最后加入 D/G/F。确认两边都只在最终输出进行一次 gamma 编码。

若法线图已经不同，先查坐标和矩阵；法线一致但高光形状不同，查 roughness 的平方次数、half-vector 和 epsilon；整体亮度比例不同，查光强、衰减、tone mapping 和 sRGB。不要用任意调曝光来掩盖接口数据错误。

## 练习与答案

将每列粗糙度固定，只让金属度逐行变化，再交换两者。预测哪些球保留明显漫反射。答案：金属度越高，模型中漫反射比例越小，镜面颜色越接近 albedo；“所有金属都更亮”不是正确结论，观察方向和环境同样重要。

迁移 IBL 的验收应分别包含 cubemap 面方向、irradiance、各粗糙度 mip、BRDF LUT 和最终组合，而不是只提交一张最终球图。当前 Vulkan/DX12 的这一部分仍是扩展任务。

<!-- generated-source-comparison -->

## 与本仓库实现并排核对

以下切片直接取自当前仓库，省略了前后文。行号用于定位，不能把片段当作完整可编译程序。

| 路线 | 可运行参照 / 范围 |
| --- | --- |
| OpenGL | [25_ibl](../../OpenGL/lessons/25_ibl/README.md) |
| Vulkan 原生 | [vk09_pbr](../../Vulkan/native/lessons/vk09_pbr/README.md) |
| Vulkan / nvpro_core2 | [nv06_pbr_viewport](../../Vulkan/nvpro/lessons/nv06_pbr_viewport/README.md) |
| DirectX 12 | [dx09_pbr](../../DirectX12/lessons/dx09_pbr/README.md) |

### OpenGL · 已实现

OpenGL 同时有 IBL；先关闭环境项再与显式后端的直接光比较。

[OpenGL/shaders/pbr.frag](../../OpenGL/shaders/pbr.frag#L43)，第 43—74 行：

```glsl
vec3 directBRDF(vec3 N, vec3 V, vec3 L, vec3 albedo, float metal, float rough, vec3 radiance) {
    vec3 H = normalize(V + L);
    float NoV = max(dot(N, V), 0.0001), NoL = max(dot(N, L), 0.0);
    vec3 F = fresnel(max(dot(V, H), 0.0), mix(vec3(0.04), albedo, metal));
    float D = distributionGGX(max(dot(N, H), 0.0), rough),
          G = geometrySchlick(NoV, rough) * geometrySchlick(NoL, rough);
    vec3 spec = D * G * F / max(4.0 * NoV * NoL, 0.0001);
    vec3 kd = (1.0 - F) * (1.0 - metal);
    return (kd * albedo / PI + spec) * radiance * NoL;
}

void main() {
    vec3 N = normalize(normal), V = normalize(eye - worldPos), albedo = baseColor;
    float metal = metallic, rough = roughness, ao = 1.0;

    // glTF 金属粗糙度贴图用 G 通道存粗糙度、B 通道存金属度，AO 单独采样。
    if (useMaps) {
        albedo *= texture(albedoMap, uv).rgb;
        vec3 mr = texture(mrMap, uv).rgb;
        metal *= mr.b;
        rough *= mr.g;
        ao = texture(aoMap, uv).r;
    }
    if (useNormal)
        N = normalize(tangentFrame(N) * (texture(normalMap, uv).xyz * 2.0 - 1.0));
    rough = clamp(rough, 0.045, 1.0);
    metal = clamp(metal, 0.0, 1.0);
    vec3 positions[4] = vec3[](vec3(-6, 6, 8), vec3(6, 6, 8), vec3(-6, -4, 6), vec3(6, -4, 6));
    vec3 result = vec3(0);
    for (int i = 0; i < 4; ++i) {
        vec3 d = positions[i] - worldPos;
        result += directBRDF(N, V, normalize(d), albedo, metal, rough, vec3(90) / dot(d, d));
```

### Vulkan 原生 · 已实现

D/G/F 和距离衰减在 GLSL 中显式计算；当前没有 IBL。

[Vulkan/common/shaders/scene.frag](../../Vulkan/common/shaders/scene.frag#L17)，第 17—42 行：

```glsl
vec3 pbr(vec3 N, vec3 V, vec3 P, float metal, float r) {
    vec3 albedo = vec3(.8, .25, .07), sum = vec3(.02) * albedo * (1.0 - metal),
         f0 = mix(vec3(.04), albedo, metal);
    for (int i = 0; i < 4; ++i) {
        vec3 lp = vec3(i % 2 == 0 ? -6 : 6, i < 2 ? 6 : -4, 8);
        vec3 delta = lp - P, L = normalize(delta), H = normalize(V + L);
        float NoV = max(dot(N, V), .001), NoL = max(dot(N, L), 0), NoH = max(dot(N, H), 0);
        float a = r * r, a2 = a * a, d = NoH * NoH * (a2 - 1) + 1,
              D = a2 / max(PI * d * d, .000001);
        float k = (r + 1) * (r + 1) / 8;
        float G = NoV / (NoV * (1 - k) + k) * NoL / (NoL * (1 - k) + k);
        vec3 F = f0 + (1 - f0) * pow(1 - max(dot(V, H), 0), 5);
        sum += ((1 - F) * (1 - metal) * albedo / PI + D * G * F / max(4 * NoV * NoL, .001)) * 90 /
               dot(delta, delta) * NoL;
    }
    return sum;
}

void main() {
    int mode = int(draw.settings.x);
    vec3 N = normalize(normal), c;
    if (mode <= 4)
        c = clamp(normal, 0, 1);
    else if (mode >= 9)
        c = pbr(
            N, normalize(vec3(0, 0, 14) - worldPos), worldPos, draw.settings.y, draw.settings.z);
```

### Vulkan / nvpro_core2 · 已实现

调粗糙度后交给相同 SceneVk；UI 和呈现路径不同。

[Vulkan/nvpro/common/nvapp_course.hpp](../../Vulkan/nvpro/common/nvapp_course.hpp#L113)，第 113—134 行：

```cpp
    void onUIRender() override {
        if (ImGui::IsKeyPressed(ImGuiKey_Escape))
            app->close();
        if (smoke)
            return;
        ImGui::Begin("Settings");
        ImGui::TextUnformatted("Vulkan / nvpro_core2 course");
        ImGui::Text("Scene mode: %d", mode);
        if (mode >= 9)
            ImGui::SliderFloat("Roughness scale", &roughnessScale, .2f, 2.f);
        ImGui::TextUnformatted("Esc: close. Resize the viewport to rebuild attachments.");
        ImGui::End();
        ImGui::Begin("Viewport");
        if (target.getSize().width)
            ImGui::Image(display, ImGui::GetContentRegionAvail());
        ImGui::End();
    }
    void onRender(VkCommandBuffer cmd) override {
        // 每次更新前等待队列，保证共用 UBO 和离屏目标不会与前一帧并发读写。
        vkCheck(vkQueueWaitIdle(c.queue));

        // GENERAL 布局不代表自动同步：仍需声明前一次读写与本次颜色写入的依赖。
```

### DirectX 12 · 已实现

对照 GLSL 的相同直接光参数；先统一空间与 gamma。

[DirectX12/shaders/scene.hlsl](../../DirectX12/shaders/scene.hlsl#L52)，第 52—79 行：

```hlsl
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
```
