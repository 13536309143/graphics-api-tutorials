# 第 19 课 · 命令录制、提交与可验证工作 · Vulkan 原生

[54 课目录](../../COURSE.md) · [对照精讲](../../../../docs/compare/09_commands_frames.md) · [上一课](../18_command_storage/README.md) · [下一课](../20_present_sync/README.md)

## 本课目标与先修

学习目标：解释“命令录制、提交与可验证工作”解决的问题，找到实际对象和参数，运行对应实验并用数据/画面证明。先完成前一课，并能说明它交付的对象与完成条件；本课在该依赖上增加新契约。

## 为什么需要这一层

命令录制发生在 CPU，执行发生在 GPU。为了不被可见窗口与光照干扰，本课先提交最小工作并验证完成标记，再把同样结构放回 draw。录制、关闭、提交、完成是四个独立节点。

## Vulkan 原生 的对象、参数与执行推导

begin/end command buffer 建立合法录制范围，QueueSubmit 接收它并关联 fence。实验填充 64 个 word，经 transfer-write→host-read 依赖后逐项核对。命令池的 family 必须匹配提交队列。

## 对照 DirectX 12：相同问题的另一种分工

CreateCommandList 初次返回可录制对象；提交前 Close。执行列表本身没有返回“工作已完成”的 HRESULT，因此要用队列 signal/fence 等待。实验检查空提交的完成条件，后续拷贝实验再检查数据。

## 关键参数与可检查不变量

| 检查项 | Vulkan 表达 | DX12 表达 | 不变量 / 证据 |
| --- | --- | --- | --- |
| 录制范围 | Begin/EndCommandBuffer | Reset/Close command list | 提交前处于合法可执行状态 |
| 提交动作 | QueueSubmit | ExecuteCommandLists | 返回不代表 GPU 工作完成 |
| 结果完成 | fence + host 可见依赖 | queue Signal + fence wait | 读取之前完成与可见性同时成立 |

先为每项指出源码中的创建/绑定位置，以及证明它成立的实际日志、字节或图像。数值中的设备限制以查询为准；固定实验尺寸不当作所有设备的上限。

## nvpro_core2 框架对照

createTempCmdBuffer 与 submitAndWaitTempCmdBuffer 适合初始化阶段的有限上传；循环中反复等待会牺牲重叠，应在掌握资源使用期后再优化。

这部分是原生 Vulkan 各课的框架对照，框架可运行入口仍为原有六课。如果本节需要新接入工作，会按正文说明；不把原生实验当作已完成的框架实现。

## 运行与实际检查

| 路线 | 实际实验 |
| --- | --- |
| Vulkan 原生 | [vk12_queue_submit](../../lessons/vk12_queue_submit/README.md) |
| DirectX 12 | [dx12_queue_submit](../../../../DirectX12/lessons/dx12_queue_submit/README.md) |

从合集根目录执行；普通图形程序 Esc 退出，设备/计算/诊断程序输出结果后结束。从“开始学习.cmd”进入本页时，表格可直接点击运行。

```powershell
cmake --preset vulkan-native
cmake --build --preset vulkan-native-debug --target vk12_queue_submit --parallel 4
.\Vulkan\native\build\bin\Debug\vk12_queue_submit.exe
ctest --preset vulkan-native-debug -R "^vk12_queue_submit$" --output-on-failure
```

检查三个层次：创建/绑定/提交的接口约束；数据读回或目标画面；资源使用结束后的回收。诊断实验用合法信息或查询证明渠道可用，不故意保留 API 错误。图形 smoke 的像素检查与数值实验的逐项检查不同，具体实现见源码。

## 关键实现：从当前源码定位

来自 [Vulkan/native/common/advanced_labs.cpp](../../common/advanced_labs.cpp#L928)，第 928—971 行。下列是当前仓库的局部切片，可能在函数中间结束；完整上下文请打开文件，切片不作为单独可编译程序。

```cpp
int nativeLabMain(int argc, char** argv, int id) {
    auto options = labOptions(argc, argv, id);
    nativeShaders = options.directory / "shaders";
    if (id == 17 || id == 19 || id == 20 || id == 21 || id == 22 || id == 23 || id == 27 ||
        id == 29 || id == 34 || id == 35 || id == 36 || id == 41 || id == 43 || id == 49 ||
        id == 50 || id == 52 || id == 53)
        return graphicsLab(argc, argv, options);
    validationErrors = 0;
    if (id == 40) {
        surfaceInspection();
        return validationErrors ? 1 : 0;
    }
    {
        NativeDevice owner(nullptr, id == 26);
        const auto& c = owner.c;
        if (id >= 37)
            inspection(c, id);
        else if (id == 11)
            capabilities(c);
        else if (id == 12) {
            VkBufferResource b;
            b.init(c, 256, VK_BUFFER_USAGE_TRANSFER_DST_BIT);
            immediate(c, [&](VkCommandBuffer cmd) {
                vkCmdFillBuffer(cmd, b.buffer, 0, 256, 0x12345678);
                bufferDependency(cmd,
                                 b.buffer,
                                 VK_PIPELINE_STAGE_2_COPY_BIT | VK_PIPELINE_STAGE_2_CLEAR_BIT,
                                 VK_ACCESS_2_TRANSFER_WRITE_BIT,
                                 VK_PIPELINE_STAGE_2_HOST_BIT,
                                 VK_ACCESS_2_HOST_READ_BIT);
            });
            for (auto x : readWords(b, 64))
                require(x == 0x12345678, "Queue fill mismatch");
            std::cout << "Verified queue submission, fence and 64 filled words\n";
        } else if (id == 13)
            stagingCopy(c);
        else if (id == 18) {
            GpuTexture texture(c, Pixels{64, 64, checker()}, true);
            texture.verify();
        } else if (id == 24)
            frameRing(c);
        else if (id == 25)
            timestamps(c);
        else if (id == 26)
```

阅读时按“前置对象 → 参数/布局 → 录制/调用 → 完成/可见性 → 释放”五步标注。共享 helper 减少重复代码，但不会取消每课的 API 契约。

## 受控修改实验

分别在录制结束、提交返回和完成等待之后设置 CPU 标记，说明哪一个允许读取结果。

先记录预测，再只改一个因素；保留基线与修改后的输出。不要同时改变 shader、资源范围和同步，这样即使结果变好也无法归因。

### 参考推导与通过标准

只有对应 GPU 完成条件成立且读取依赖正确时允许读取。前两个标记描述 CPU 进度，不证明 GPU 已写完。

## 常见失败与排查顺序

漏掉 End/Close 或重用仍在执行的命令存储，应优先查看验证信息，而不是调整绘制几何。

先看第一条错误和当前执行路径，再看实际数据/绑定，最后看 shader 与画面。需要 GPU 证据时抓取正常交互帧；仅四帧的 smoke 适合自动检查，不适合手动抓帧。

## 自测与本课交付

能否不看源码回答：分别在录制结束、提交返回和完成等待之后设置 CPU 标记，说明哪一个允许读取结果。

提交一份简短记录：输入与假设、唯一修改、预期、实际证据、原因、是否恢复基线。如果只能复述函数名而说不清何时允许访问/释放，先回到本课的数据流。

## 官方阅读位置与当前工程差异

[指定 Vulkan 教程对应位置](https://docs.vulkan.net.cn/tutorial/latest/03_Drawing_a_triangle/03_Drawing/01_Command_buffers.html)。DX12 路线按同一概念顺序建立映射，其对象与约束以 Microsoft 文档和本后端代码为准。49—54 课包含在基础/compute 之后加入的工程扩展。

[版本与资料说明](../../../../docs/REFERENCES.md) · [调试](../../../../docs/DEBUGGING.md) · [功能完成度](../../../../docs/FEATURE_MATRIX.md)
