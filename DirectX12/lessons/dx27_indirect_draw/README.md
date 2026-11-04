# dx27_indirect_draw · 间接绘制

[54 课目录](../../COURSE.md)

## 目的与实际执行

D3D12_DRAW_ARGUMENTS 配合 command signature 的 DRAW 类型与 ByteStride，再由 ExecuteIndirect 解释。signature 可声明其他绑定变更，但本例只控制 draw 参数，不改 root arguments。

本程序有独立实验选择与验证路径，公共实现集中在 advanced_labs.cpp。设备/API 检查、数值 compute 与图形渲染采用不同证据；不是每个程序都会弹出窗口。

## 构建和运行

```powershell
cmake --preset dx12
cmake --build --preset dx12-debug --target dx27_indirect_draw --parallel 4
.\DirectX12\build\bin\Debug\dx27_indirect_draw.exe
```

## 深入学习：数据、接口与执行

- [第 51 课：间接参数、命令签名与可扩展提交](../../chapters/51_indirect/README.md)

### 本课的真实源码切片

来自 [DirectX12/common/dx12_course.hpp](../../common/dx12_course.hpp#L443)，第 443—486 行；这是现有实现的局部摘录，末尾可能处于函数中间，不是可单独编译的完整程序。

```cpp
        if (mode >= 4) {
            cmd->IASetVertexBuffers(0, 1, &vb);
            if (indices)
                cmd->IASetIndexBuffer(&ib);
        }
        for (auto d : overrideDraws.empty() ? draws(mode, time) : overrideDraws) {
            cmd->SetGraphicsRoot32BitConstants(1, 20, &d, 0);
            if (indirectArguments)
                cmd->ExecuteIndirect(
                    indirectSignature.Get(), 1, indirectArguments.Get(), 0, nullptr, 0);
            else if (mode == 3)
                cmd->DrawInstanced(3, 1, 0, 0);
            else if (mode == 4)
                cmd->DrawInstanced(UINT(mesh.vertices.size()), 1, 0, 0);
            else
                cmd->DrawIndexedInstanced(UINT(mesh.indices.size()), mode == 7 ? 25 : 1, 0, 0, 0);
        }
    }
};
struct DxRenderOptions {
    const Geometry* mesh = nullptr;
    D3D12_CULL_MODE cull = D3D12_CULL_MODE_NONE;
    bool clampSampler = false, redOnly = false;
    std::string captureStem;
    UINT samples = 1;
    std::function<void(DxScene&)> setup;
    std::function<void(DxScene&, float)> update;
    std::function<void(DxScene&, float)> beforeFrame;
};
class DxWindow {
public:
    DxDevice c;
    GLFWwindow* window;
    int mode, width = 0, height = 0;
    ComPtr<IDXGISwapChain3> swap;
    ComPtr<ID3D12DescriptorHeap> rtvHeap, dsvHeap;
    ComPtr<ID3D12Resource> colors[2], depth, offscreen, multisample;
    DxRenderOptions options;
    std::unique_ptr<DxScene> scene;
    UINT rtvStride = 0;
    DxWindow(GLFWwindow* w, int lesson, bool warp, DxRenderOptions settings = {})
        : c(warp), window(w), mode(lesson), options(std::move(settings)) {
        D3D12_DESCRIPTOR_HEAP_DESC heap{};
        heap.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
```

### 同主题跨 API 导航

| 路线 | 程序 |
| --- | --- |
| DirectX 12 | [dx27_indirect_draw](README.md) |
| 对照后端 | [vk27_indirect_draw](../../../Vulkan/native/lessons/vk27_indirect_draw/README.md) |

### 先预测，再验证

把 vertex count 改为零，预测为何应该没有三角形，并说明它与缓冲为空的区别。

参考推导：合法参数要求零工作，与资源或绑定无效不是一回事。恢复 count 后应重新出现；错误句柄不应该被当作“禁用绘制”手段。

## 验证与边界

运行 `--smoke` 执行有限自动检查。数值实验读取实际 GPU 结果，图形实验检查非空图像并可查看读回。交互重建与性能还需额外证据；窗口 renderer 当前保留串行等待，双槽计算实验不会被描述为完整多帧呈现引擎。

## 实际画面

![dx27_indirect_draw](../../../docs/previews/dx27_indirect_draw.png)
