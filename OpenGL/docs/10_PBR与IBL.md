# PBR / IBL：把公式与数据流连起来

## 直接光

渲染方程的反射部分把来自半球各方向的入射辐射度 Li 与 BRDF、cosθ 相乘后积分。离散点光源可以直接逐灯求和；环境光则需要方向积分的近似。

```text
fr = kd * albedo / PI + D * G * F / (4 * NoV * NoL)
Lo_direct += fr * radiance * NoL
F0 = mix(0.04, albedo, metallic)
kd = (1 - F) * (1 - metallic)
```

GGX 的 D 描述微表面法线分布，G 描述微表面遮挡/掩蔽，F 描述视角相关 Fresnel。roughness 不是镜面强度滑块；它改变高光和环境反射的分布。金属度=1 时没有本模型中的漫反射，颜色来自有色镜面响应。

本课程使用单次散射 Cook-Torrance、GGX/Schlick 近似，没有多次散射补偿、面积光精确积分、复杂介质和薄膜。PBR 不保证“填上几个参数就像照片”，环境、尺度、曝光、材质和几何都必须可信。

## 间接光 / IBL

```text
studio.hdr (linear RGBE)
  → equirectangular to cubemap (RGB16F)
  → cosine hemisphere integration → irradiance cubemap
  → GGX importance sampling at 5 roughness values → prefiltered cubemap mips
  → numerical BRDF integration → RG16F LUT (NdotV, roughness)

diffuse_env = irradiance * albedo / PI
spec_env = prefiltered(R, roughness) * (F0 * LUT.x + LUT.y)
```

环境反射也受金属/粗糙度与 Fresnel 控制。普通 mipmap 与反射分布预积分不同；本教程逐 mip 真正进行 GGX 采样。为了快速学习，环境 128²、漫反射 16²、镜面 64² 五级、LUT 128²，每项 128 样本。提高品质时同步修改采样序列分母、循环次数、纹理大小与运行时最大 LOD。

## 色彩空间契约

| 数据 | 文件/上传格式 | shader 中的含义 |
| --- | --- | --- |
| albedo/base color | PNG + GL_SRGB8_ALPHA8 | GPU 解码到线性颜色 |
| normal / metallic / roughness / AO | GL_RGBA8 | 线性数值，不能 Gamma |
| HDR 环境 | stbi_loadf + RGB16F | 线性、可超过 1 |
| G-buffer position/normal | 浮点 | 几何数据 |
| HDR scene/Bloom | RGBA16F | 线性辐射亮度近似 |
| 最终窗口输出 | tone mapping + 简化 Gamma | 显示编码值 |

23–25 为独立展示，在 PBR shader 中完成最终映射；34 接入 HDR，必须关闭 PBR 与天空的局部编码，最后只输出编码一次。灯光值与材质因子都是线性数，不要用图片编辑软件看到的 sRGB 数字直接当物理反射率。

## 验证方法

固定相机、灯和环境，只改一个变量。横向看 roughness 对高光宽度的影响，纵向看 metallic 对漫反射的关闭；关闭 IBL 看未被直接光照亮的金属是否失去环境反射；暂停动画对比 bind pose。不要仅根据“画面更亮”认定 PBR 实现正确。

深入推导可以阅读 [Filament 技术说明](https://google.github.io/filament/Filament.md.html)，再回到本工程逐项核对所选近似和数值范围；生产引擎的模型比本课更完整。
