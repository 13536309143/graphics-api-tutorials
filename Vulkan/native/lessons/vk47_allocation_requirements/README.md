# vk47_allocation_requirements · 分配大小与对齐

[54 课目录](../../COURSE.md)

## 目的与实际执行

memoryTypeBits 是资源允许的类型集合，propertyFlags 是内存属性。实验为 257 字节缓冲查询实际 size/alignment/type bits，再在允许集合中选择所需属性。non-coherent 的 flush/invalidate 还需符合原子尺寸对齐。

本程序有独立实验选择与验证路径，公共实现集中在 advanced_labs.cpp。设备/API 检查、数值 compute 与图形渲染采用不同证据；不是每个程序都会弹出窗口。

## 构建和运行

```powershell
cmake --preset vulkan-native
cmake --build --preset vulkan-native-debug --target vk47_allocation_requirements --parallel 4
.\Vulkan\native\build\bin\Debug\vk47_allocation_requirements.exe
```

## 深入学习：数据、接口与执行

- [第 25 课：内存类型、Heap 与分配要求](../../chapters/25_memory/README.md)

### 本课的真实源码切片

来自 [Vulkan/common/scene_vk.hpp](../../../common/scene_vk.hpp#L52)，第 52—95 行；这是现有实现的局部摘录，末尾可能处于函数中间，不是可单独编译的完整程序。

```cpp
        vkGetBufferMemoryRequirements(device, buffer, &req);
        VkMemoryAllocateInfo alloc{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
        alloc.allocationSize = req.size;
        alloc.memoryTypeIndex = memoryType(c, req.memoryTypeBits, flags);
        vkCheck(vkAllocateMemory(device, &alloc, nullptr, &memory));
        vkCheck(vkBindBufferMemory(device, buffer, memory, 0));
    }

    // 此处缓冲使用 HOST_VISIBLE | HOST_COHERENT；非 coherent 内存还需显式 flush。
    void upload(const void* data, size_t bytes) {
        void* p;
        vkCheck(vkMapMemory(device, memory, 0, size, 0, &p));
        std::memcpy(p, data, bytes);
        vkUnmapMemory(device, memory);
    }
    ~VkBufferResource() {
        if (buffer)
            vkDestroyBuffer(device, buffer, nullptr);
        if (memory)
            vkFreeMemory(device, memory, nullptr);
    }
};
struct VkImageResource {
    VkDevice device{};
    VkImage image{};
    VkImageView view{};
    VkDeviceMemory memory{};
    VkImageResource() = default;
    VkImageResource(const VkImageResource&) = delete;
    VkImageResource& operator=(const VkImageResource&) = delete;
    void init(const VkContextRef& c,
              VkExtent2D size,
              VkFormat format,
              VkImageUsageFlags usage,
              VkImageAspectFlags aspect,
              uint32_t levels = 1,
              VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT) {
        device = c.device;
        VkImageCreateInfo info{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
        info.imageType = VK_IMAGE_TYPE_2D;
        info.format = format;
        info.extent = {size.width, size.height, 1};
        info.mipLevels = levels;
        info.arrayLayers = 1;
```

### 同主题跨 API 导航

| 路线 | 程序 |
| --- | --- |
| Vulkan 原生 | [vk47_allocation_requirements](README.md) |
| 对照后端 | [dx47_allocation_requirements](../../../../DirectX12/lessons/dx47_allocation_requirements/README.md) |

### 先预测，再验证

比较请求 257 字节、返回的 allocation size、常量槽 stride 三者，解释不同来源。

参考推导：257 是应用内容大小，allocation size 来自内存分配约束，常量 stride 来自接口偏移对齐。三者可以不同而同时正确。

## 验证与边界

运行 `--smoke` 执行有限自动检查。数值实验读取实际 GPU 结果，图形实验检查非空图像并可查看读回。交互重建与性能还需额外证据；窗口 renderer 当前保留串行等待，双槽计算实验不会被描述为完整多帧呈现引擎。
