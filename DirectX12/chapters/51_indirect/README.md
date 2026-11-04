# 第 51 课 · 间接参数、命令签名与可扩展提交 · DirectX 12

[54 课目录](../../COURSE.md) · [对照精讲](../../../docs/compare/09_commands_frames.md) · [上一课](../50_timeline/README.md) · [下一课](../52_camera_material/README.md)

## 本课目标与先修

学习目标：解释“间接参数、命令签名与可扩展提交”解决的问题，找到实际对象和参数，运行对应实验并用数据/画面证明。先完成前一课，并能说明它交付的对象与完成条件；本课在该依赖上增加新契约。

## 为什么需要这一层

indirect 将 draw 参数放在 GPU 可访问缓冲，调用命令读取它。它与“顶点来自 buffer”不同：这里缓冲控制绘制数量和起始位置。当前参数由 CPU 填充；GPU 剔除和生成命令是后续扩展，不冒充已完成。

## DirectX 12 的对象、参数与执行推导

D3D12_DRAW_ARGUMENTS 配合 command signature 的 DRAW 类型与 ByteStride，再由 ExecuteIndirect 解释。signature 可声明其他绑定变更，但本例只控制 draw 参数，不改 root arguments。

## 对照 Vulkan 原生：相同问题的另一种分工

VkDrawIndirectCommand 的四个 uint 字段与 stride 匹配，缓冲包含 INDIRECT_BUFFER usage，vkCmdDrawIndirect 读取参数。若以后 compute 写参数，需要 compute-write→draw-indirect-read 的依赖。

## 关键参数与可检查不变量

| 检查项 | Vulkan 表达 | DX12 表达 | 不变量 / 证据 |
| --- | --- | --- | --- |
| 间接字节 | DrawIndirectCommand 4 uint | DRAW_ARGUMENTS 4 uint | 16-byte 参数布局 |
| 解释接口 | vkCmdDrawIndirect | command signature + ExecuteIndirect | stride/type 与资源内容一致 |
| 生产者范围 | 当前 CPU 写参数 | 当前 CPU 写参数 | GPU 剔除/命令生成尚未接入 |

先为每项指出源码中的创建/绑定位置，以及证明它成立的实际日志、字节或图像。数值中的设备限制以查询为准；固定实验尺寸不当作所有设备的上限。

## nvpro_core2 框架对照

相同 SceneVk 可以接入 indirectArguments，框架仍负责外围目标/帧循环。不能只创建一个 indirect buffer 而继续走普通 Draw 就算完成该特性。

这部分是原生 Vulkan 各课的框架对照，框架可运行入口仍为原有六课。如果本节需要新接入工作，会按正文说明；不把原生实验当作已完成的框架实现。

## 运行与实际检查

| 路线 | 实际实验 |
| --- | --- |
| DirectX 12 | [dx27_indirect_draw](../../lessons/dx27_indirect_draw/README.md) |
| Vulkan 原生 | [vk27_indirect_draw](../../../Vulkan/native/lessons/vk27_indirect_draw/README.md) |

从合集根目录执行；普通图形程序 Esc 退出，设备/计算/诊断程序输出结果后结束。从“开始学习.cmd”进入本页时，表格可直接点击运行。

```powershell
cmake --preset dx12
cmake --build --preset dx12-debug --target dx27_indirect_draw --parallel 4
.\DirectX12\build\bin\Debug\dx27_indirect_draw.exe
ctest --preset dx12-debug -R "^dx27_indirect_draw$" --output-on-failure
```

检查三个层次：创建/绑定/提交的接口约束；数据读回或目标画面；资源使用结束后的回收。诊断实验用合法信息或查询证明渠道可用，不故意保留 API 错误。图形 smoke 的像素检查与数值实验的逐项检查不同，具体实现见源码。

## 关键实现：从当前源码定位

来自 [DirectX12/common/dx12_course.hpp](../../common/dx12_course.hpp#L443)，第 443—486 行。下列是当前仓库的局部切片，可能在函数中间结束；完整上下文请打开文件，切片不作为单独可编译程序。

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

阅读时按“前置对象 → 参数/布局 → 录制/调用 → 完成/可见性 → 释放”五步标注。共享 helper 减少重复代码，但不会取消每课的 API 契约。

## 受控修改实验

把 vertex count 改为零，预测为何应该没有三角形，并说明它与缓冲为空的区别。

先记录预测，再只改一个因素；保留基线与修改后的输出。不要同时改变 shader、资源范围和同步，这样即使结果变好也无法归因。

### 参考推导与通过标准

合法参数要求零工作，与资源或绑定无效不是一回事。恢复 count 后应重新出现；错误句柄不应该被当作“禁用绘制”手段。

## 常见失败与排查顺序

结构字节布局、offset 对齐或 command signature stride 错误，会让 GPU 解释错误命令。先核对参数字节，再考虑 GPU-driven 调度。

先看第一条错误和当前执行路径，再看实际数据/绑定，最后看 shader 与画面。需要 GPU 证据时抓取正常交互帧；仅四帧的 smoke 适合自动检查，不适合手动抓帧。

## 自测与本课交付

能否不看源码回答：把 vertex count 改为零，预测为何应该没有三角形，并说明它与缓冲为空的区别。

提交一份简短记录：输入与假设、唯一修改、预期、实际证据、原因、是否恢复基线。如果只能复述函数名而说不清何时允许访问/释放，先回到本课的数据流。

## 官方阅读位置与当前工程差异

[指定 Vulkan 教程对应位置](https://docs.vulkan.net.cn/tutorial/latest/11_Compute_Shader.html)。DX12 路线按同一概念顺序建立映射，其对象与约束以 Microsoft 文档和本后端代码为准。49—54 课包含在基础/compute 之后加入的工程扩展。

[版本与资料说明](../../../docs/REFERENCES.md) · [调试](../../../docs/DEBUGGING.md) · [功能完成度](../../../docs/FEATURE_MATRIX.md)

## 固定输入的实际 GPU 读回

![dx27_indirect_draw](../../../docs/previews/dx27_indirect_draw.png)
