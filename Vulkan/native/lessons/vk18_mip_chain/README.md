# vk18_mip_chain · GPU mip 链与逐级读回

[54 课目录](../../COURSE.md)

## 目的与实际执行

copy-write 到 fragment-sampled-read 需要对应 stage/access 和布局转换。mip 生成逐级把前一级从 transfer-dst 切成 transfer-src，最终各级转 shader-read；不能仅在整个流程结束后随意改一个枚举。

查询格式的 blit/filter 支持，上传 level 0，逐级 transition 和 linear blit，再把所有级别转成 shader-read。每次只读取已经完成的前一级，最后复制每级结果用于校验。数值参考使用线性 UNORM。

本程序有独立实验选择与验证路径，公共实现集中在 advanced_labs.cpp。设备/API 检查、数值 compute 与图形渲染采用不同证据；不是每个程序都会弹出窗口。

## 构建和运行

```powershell
cmake --preset vulkan-native
cmake --build --preset vulkan-native-debug --target vk18_mip_chain --parallel 4
.\Vulkan\native\build\bin\Debug\vk18_mip_chain.exe
```

## 深入学习：数据、接口与执行

- [第 35 课：图像布局、资源状态与读写依赖](../../chapters/35_image_dependencies/README.md)
- [第 43 课：GPU Mip 链生成与逐级验收](../../chapters/43_mip_generation/README.md)

### 本课的真实源码切片

来自 [Vulkan/native/common/advanced_labs.cpp](../../common/advanced_labs.cpp#L409)，第 409—452 行；这是现有实现的局部摘录，末尾可能处于函数中间，不是可单独编译的完整程序。

```cpp
                                VK_ACCESS_2_TRANSFER_WRITE_BIT,
                                VK_PIPELINE_STAGE_2_BLIT_BIT,
                                VK_ACCESS_2_TRANSFER_READ_BIT);
                VkImageBlit blit{};
                blit.srcSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, i - 1, 0, 1};
                blit.dstSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, i, 0, 1};
                blit.srcOffsets[1] = {int(reference[i - 1].width), int(reference[i - 1].height), 1};
                blit.dstOffsets[1] = {int(reference[i].width), int(reference[i].height), 1};
                vkCmdBlitImage(cmd,
                               image.image,
                               VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                               image.image,
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                               1,
                               &blit,
                               VK_FILTER_LINEAR);
            }
            for (uint32_t i = 0; i < levels; ++i)
                imageDependency(cmd,
                                image.image,
                                i,
                                i + 1 == levels ? VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
                                                : VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                VK_PIPELINE_STAGE_2_COPY_BIT | VK_PIPELINE_STAGE_2_BLIT_BIT,
                                VK_ACCESS_2_TRANSFER_READ_BIT | VK_ACCESS_2_TRANSFER_WRITE_BIT,
                                VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
                                VK_ACCESS_2_SHADER_SAMPLED_READ_BIT);
        });
        VkSamplerCreateInfo si{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
        si.magFilter = si.minFilter = VK_FILTER_LINEAR;
        si.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        si.addressModeU = si.addressModeV = si.addressModeW =
            clamp ? VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE : VK_SAMPLER_ADDRESS_MODE_REPEAT;
        si.maxLod = float(levels - 1);
        vkCheck(vkCreateSampler(c.device, &si, nullptr, &sampler));
        std::cout << "Uploaded image " << pixels.width << 'x' << pixels.height
                  << ", mipLevels=" << levels << ", sRGB=" << srgb << '\n';
    }
    void attach(SceneVk& scene, bool lastMip = false) {
        if (lastMip) {
            VkImageViewCreateInfo ci{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
            ci.image = image.image;
            ci.viewType = VK_IMAGE_VIEW_TYPE_2D;
```

### 同主题跨 API 导航

| 路线 | 程序 |
| --- | --- |
| Vulkan 原生 | [vk18_mip_chain](README.md) |
| 对照后端 | [dx18_mip_chain](../../../../DirectX12/lessons/dx18_mip_chain/README.md) |

### 先预测，再验证

把纹理消费者从 pixel/fragment 改为 compute，哪些同步描述也要一起改变？

参考推导：消费者阶段/访问或 state 应反映 compute 读取。直接复用只针对片元的依赖不一定覆盖新的使用者。

把基图改为棋盘或分区整数颜色，为什么比任意照片更适合作为 mip 数值验收？

参考推导：可手算局部平均，并发现级数、偏移或尾部错误。图像“看着模糊了”不足以证明每一级都正确。

## 验证与边界

运行 `--smoke` 执行有限自动检查。数值实验读取实际 GPU 结果，图形实验检查非空图像并可查看读回。交互重建与性能还需额外证据；窗口 renderer 当前保留串行等待，双槽计算实验不会被描述为完整多帧呈现引擎。
