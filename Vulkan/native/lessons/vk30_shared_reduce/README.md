# vk30_shared_reduce · 组共享内存归约

[54 课目录](../../COURSE.md)

## 目的与实际执行

shared 数组在组内可见，barrier 保证迭代间写入/读取顺序。不同工作组没有可直接互相等待的全局 barrier，本课把跨组聚合留到独立阶段。

本程序有独立实验选择与验证路径，公共实现集中在 advanced_labs.cpp。设备/API 检查、数值 compute 与图形渲染采用不同证据；不是每个程序都会弹出窗口。

## 构建和运行

```powershell
cmake --preset vulkan-native
cmake --build --preset vulkan-native-debug --target vk30_shared_reduce --parallel 4
.\Vulkan\native\build\bin\Debug\vk30_shared_reduce.exe
```

## 深入学习：数据、接口与执行

- [第 49 课：组共享内存、归约与屏障一致性](../../chapters/49_group_shared/README.md)

### 本课的真实源码切片

来自 [Vulkan/common/shaders/lab.comp](../../../common/shaders/lab.comp#L19)，第 19—38 行；这是现有实现的局部摘录，末尾可能处于函数中间，不是可单独编译的完整程序。

```glsl
void main() {
    uint i = gl_GlobalInvocationID.x;
    uint lane = gl_LocalInvocationID.x;
    if (operation.mode == 2) {
        // All invocations reach each barrier, including padded tail lanes.
        partial[lane] = i < operation.count ? i + 1 : 0;
        barrier();
        for (uint stride = 32; stride > 0; stride >>= 1) {
            if (lane < stride)
                partial[lane] += partial[lane + stride];
            barrier();
        }
        if (lane == 0)
            outputData.values[gl_WorkGroupID.x] = partial[0];
    } else if (i < operation.count) {
        uint factor = operation.mode == 1 ? parameters.values.x : operation.factor;
        uint bias = operation.mode == 1 ? parameters.values.y : operation.bias;
        outputData.values[i] = i * factor + bias;
    }
}
```

### 同主题跨 API 导航

| 路线 | 程序 |
| --- | --- |
| Vulkan 原生 | [vk30_shared_reduce](README.md) |
| 对照后端 | [dx30_shared_reduce](../../../../DirectX12/lessons/dx30_shared_reduce/README.md) |

### 先预测，再验证

若尾部 lane 在第一个 barrier 前直接 return，为什么不是一种安全优化？

参考推导：组中部分线程仍在等待完整同步，屏障的控制流条件不再成立。应让它们贡献零并继续参与屏障。

## 验证与边界

运行 `--smoke` 执行有限自动检查。数值实验读取实际 GPU 结果，图形实验检查非空图像并可查看读回。交互重建与性能还需额外证据；窗口 renderer 当前保留串行等待，双槽计算实验不会被描述为完整多帧呈现引擎。
