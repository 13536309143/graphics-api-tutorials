# dx53_vertex_layout · 顶点布局检验

[54 课目录](../../COURSE.md)

## 目的与实际执行

VBV 给 GPU 地址、大小和 stride，input layout 用 POSITION/NORMAL/TEXCOORD 连接 HLSL 输入。语义名称与偏移分别检查，不能因为名字相同就忽略格式。实例数据还要声明独立的输入分类/步进。

本程序有独立实验选择与验证路径，公共实现集中在 advanced_labs.cpp。设备/API 检查、数值 compute 与图形渲染采用不同证据；不是每个程序都会弹出窗口。

## 构建和运行

```powershell
cmake --preset dx12
cmake --build --preset dx12-debug --target dx53_vertex_layout --parallel 4
.\DirectX12\build\bin\Debug\dx53_vertex_layout.exe
```

## 深入学习：数据、接口与执行

- [第 23 课：顶点语义、Stride 与 Offset](../../chapters/23_vertex_input/README.md)

### 本课的真实源码切片

来自 [DirectX12/common/dx12_course.hpp](../../common/dx12_course.hpp#L376)，第 376—419 行；这是现有实现的局部摘录，末尾可能处于函数中间，不是可单独编译的完整程序。

```cpp
        D3D12_INPUT_ELEMENT_DESC elements[] = {{"POSITION",
                                                0,
                                                DXGI_FORMAT_R32G32B32_FLOAT,
                                                0,
                                                0,
                                                D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
                                                0},
                                               {"NORMAL",
                                                0,
                                                DXGI_FORMAT_R32G32B32_FLOAT,
                                                0,
                                                12,
                                                D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
                                                0},
                                               {"TEXCOORD",
                                                0,
                                                DXGI_FORMAT_R32G32_FLOAT,
                                                0,
                                                24,
                                                D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
                                                0}};
        D3D12_GRAPHICS_PIPELINE_STATE_DESC pso{};
        pso.pRootSignature = signature.Get();
        pso.VS = {vs->GetBufferPointer(), vs->GetBufferSize()};
        pso.PS = {ps->GetBufferPointer(), ps->GetBufferSize()};
        pso.BlendState.RenderTarget[0].RenderTargetWriteMask =
            redOnly ? D3D12_COLOR_WRITE_ENABLE_RED | D3D12_COLOR_WRITE_ENABLE_ALPHA
                    : D3D12_COLOR_WRITE_ENABLE_ALL;
        pso.SampleMask = UINT_MAX;
        pso.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
        pso.RasterizerState.CullMode = cull;
        // 共用模型按外表面逆时针排列；明确指定正面，避免剔除后只剩内部。
        pso.RasterizerState.FrontCounterClockwise = TRUE;
        pso.RasterizerState.DepthClipEnable = TRUE;
        pso.DepthStencilState.DepthEnable = mode >= 5;
        pso.DepthStencilState.DepthWriteMask =
            mode >= 5 ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
        pso.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
        pso.DepthStencilState.StencilReadMask = pso.DepthStencilState.StencilWriteMask = 0xFF;
        if (mode >= 4)
            pso.InputLayout = {elements, 3};
        pso.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        pso.NumRenderTargets = 1;
        pso.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
```

### 同主题跨 API 导航

| 路线 | 程序 |
| --- | --- |
| DirectX 12 | [dx53_vertex_layout](README.md) |
| 对照后端 | [vk53_vertex_layout](../../../Vulkan/native/lessons/vk53_vertex_layout/README.md) |

### 先预测，再验证

把某一个属性偏移故意改错，先预测影响的是形状、颜色还是 UV，再用 mesh viewer 查字节。

参考推导：位置错会改变几何，颜色/法线错影响插值或光照，UV 错影响采样。一次只改一项才能判断其责任。

## 验证与边界

运行 `--smoke` 执行有限自动检查。数值实验读取实际 GPU 结果，图形实验检查非空图像并可查看读回。交互重建与性能还需额外证据；窗口 renderer 当前保留串行等待，双槽计算实验不会被描述为完整多帧呈现引擎。

## 实际画面

![dx53_vertex_layout](../../../docs/previews/dx53_vertex_layout.png)
