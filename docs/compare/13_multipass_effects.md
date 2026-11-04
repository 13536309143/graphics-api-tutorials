# 13 · 阴影、延迟渲染、SSAO 与后处理依赖图

[对照路线](../COMPARE.md) · [交互式对照](index.html#13_multipass_effects) · [上一章](12_lighting_pbr_ibl.md) · [下一章](14_debug_performance.md)

高级效果通常是多个简单 pass 加上正确的数据流。跨 API 迁移时，先把“哪一遍生成什么，下一遍如何读取”画出来，比直接翻译所有创建函数更可靠。

## 一个最小依赖图

```text
几何 / 相机 ──> G-buffer: 位置、法线、材质
                    ├──> SSAO ──> 双边降噪 ──┐
                    └────────────────────────> 光照 ──> HDR
阴影深度 ────────────────────────────────────┘        │
                                      高亮提取 -> 模糊 │
                                             └──> 合成/显示编码
```

这是一种组织示意，并非当前所有后端共享的完整执行图。OpenGL 26/27 实现 G-buffer、延迟光照与 SSAO；Vulkan/DX12 尚未实现对应全套效果。它们的 offscreen 课提供目标与传输的起点。

## 阴影：从另一个相机保存可见性

方向光 shadow mapping 先从光源视角输出深度，再将世界位置投影到光空间比较。self-shadow acne 来自离散采样和精度等因素，bias 太大又会造成物体与阴影分离。PCF 平均多次深度比较，改善的是边缘，不会修复错误投影、错误 sampler 或深度范围。

OpenGL 33 的点光阴影使用六个方向的 depth cubemap，并存归一化径向距离。它与普通透视 depth buffer 的非线性深度不同，比较端必须采用同一距离定义。迁移时可先六次绘制，确认面方向正确，再考虑 layered rendering 等优化。

## 延迟渲染：把几何与光照分开

G-buffer 把位置、法线、albedo 等保存到多个附件。光照遍只处理可见像素，减少大量光源重复执行几何，但增加带宽和存储开销。透明物体通常另用 forward pass；不同材质需要额外参数编码，不能说“deferred 永远比 forward 快”。

当前 OpenGL G-buffer 将位置/法线存到 view space，因此 SSAO 也在 view space 采样，不能拿 world-space 光源直接混算。位置也可由深度重构来节省带宽，但必须使用正确的投影逆矩阵和 API 深度约定；这在当前项目中属于进一步实验。

## SSAO：先理解近似，再看噪声

半球样本按法线定向，投影到屏幕后与邻域位置比较。随机旋转减少固定方向条纹，却产生颗粒；降噪还要看位置与法线相似度，避免跨物体轮廓糊成黑边。radius 和 bias 与场景尺度相关，不是适用于任意单位的魔法常数。

SSAO 不会恢复屏幕外遮挡，也不是完整全局照明。验收时同时查看位置、法线、原始 AO、降噪 AO 和最终光照，才能判断黑斑来自几何数据、采样还是合成。

## 三种 API 如何表达 pass 的连接

| 连接 | OpenGL | Vulkan 迁移方案 | DX12 迁移方案 |
| --- | --- | --- | --- |
| G-buffer 写后采样 | 切 FBO 并绑定纹理，避免反馈 | color-write→sampled-read barrier | RENDER_TARGET→shader resource |
| 阴影深度写后读取 | depth texture + 比较采样 | depth-write→sampled-read 与布局 | DEPTH_WRITE→对应 shader-read state |
| ping-pong 模糊 | 交替两个纹理/FBO | 两图交替并追踪每遍依赖 | 两资源交替并转换状态 |
| MSAA resolve | blit resolve | resolve attachment/command | ResolveSubresource 等 |

不要在同一个 pass 不加区分地同时把同一子资源当颜色输出与普通采样输入。先使用两个目标建立正确 ping-pong，再研究合法的高级反馈机制。

## 练习：先迁移一种最简单后处理

为现有 vk10/dx10 增加全屏采样并输出灰度，替代 blit/copy。列出新的 sampler/SRV、shader-read 用途、管线、descriptor/root binding 与颜色写后读依赖。验收先看原色采样无变化，再启用灰度；若原色已错，暂时不要加 Bloom。该方案明确是待实现练习。

<!-- generated-source-comparison -->

## 与本仓库实现并排核对

以下切片直接取自当前仓库，省略了前后文。行号用于定位，不能把片段当作完整可编译程序。

| 路线 | 可运行参照 / 范围 |
| --- | --- |
| OpenGL | [27_ssao](../../OpenGL/lessons/27_ssao/README.md) |
| Vulkan 原生 | 迁移设计 |
| Vulkan / nvpro_core2 | 迁移设计 |
| DirectX 12 | 迁移设计 |

### OpenGL · 已实现

先有 G-buffer，再估算遮挡、降噪，最后合成环境光。

[OpenGL/lessons/27_ssao/main.cpp](../../OpenGL/lessons/27_ssao/main.cpp#L70)，第 70—116 行：

```cpp
        // 第二遍：从 G-buffer 和噪声构造采样方向，估算局部遮挡。
        occlusion.bind();
        ao.use();
        ao.set("positionTex", 0);
        ao.set("normalTex", 1);
        ao.set("noiseTex", 2);
        ao.set("projection", camera.projection(app));
        for (int k = 0; k < 32; ++k)
            ao.set(("samples[" + std::to_string(k) + "]").c_str(), kernel[k]);
        bind2D(gbuffer.colors[0], 0);
        bind2D(gbuffer.colors[1], 1);
        bind2D(noiseTexture, 2);
        plane.draw();

        // 第三遍：按几何边界降噪，减少随机样本产生的颗粒感。
        filtered.bind();
        blur.use();
        blur.set("tex", 0);
        blur.set("positionTex", 1);
        blur.set("normalTex", 2);
        bind2D(occlusion.colors[0], 0);
        bind2D(gbuffer.colors[0], 1);
        bind2D(gbuffer.colors[1], 2);
        plane.draw();

        // 最后一遍：合成光照；AO 仅调制环境项，可用按键查看中间结果。
        defaultTarget(app);
        app.clear();
        lighting.use();
        lighting.set("positionTex", 0);
        lighting.set("normalTex", 1);
        lighting.set("albedoTex", 2);
        lighting.set("aoTex", 3);
        lighting.set("view", camera.view());
        lighting.set("useAO", app.key(GLFW_KEY_SPACE) != GLFW_PRESS);
        lighting.set("debugView",
                     glfwGetKey(app.window, GLFW_KEY_1) == GLFW_PRESS   ? 1
                     : glfwGetKey(app.window, GLFW_KEY_2) == GLFW_PRESS ? 2
                                                                        : 0);
        bind2D(gbuffer.colors[0], 0);
        bind2D(gbuffer.colors[1], 1);
        bind2D(gbuffer.colors[2], 2);
        bind2D(filtered.colors[0], 3);
        plane.draw();
        app.end();
    }
    glDeleteTextures(1, &noiseTexture);
```

### Vulkan 原生 · 迁移设计

vk10 已有离屏/blit；完整 Bloom/SSAO 仍需资源、采样管线及逐遍依赖。


### Vulkan / nvpro_core2 · 迁移设计

RenderTarget 与 ImGui 仅是接入点；新增多遍效果仍要自己描述写后读。


### DirectX 12 · 迁移设计

dx10 是 CopyResource，迁移后处理需新增可采样目标和全屏像素管线。
