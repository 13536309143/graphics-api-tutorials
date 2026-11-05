# dx12_queue_submit · 提交与完成条件

[54 课目录](../../COURSE.md)

## 本课独立工程与关键注释

[main.cpp](main.cpp) · [本地辅助实现](support/) · [本课着色器](shaders/) · [CMakeLists.txt](CMakeLists.txt)

本目录包含独立课程源码，关键位置有中文注释。单独复制整个文件夹后可用 `cmake -S . -B build -A x64` 配置、`cmake --build build --config Debug` 构建；不需要其他章节源码。

## 目的与实际执行

CreateCommandList 初次返回可录制对象；提交前 Close。执行列表本身没有返回“工作已完成”的 HRESULT，因此要用队列 signal/fence 等待。实验检查空提交的完成条件，后续拷贝实验再检查数据。

本程序的 C++、辅助实现、着色器和构建脚本均位于本目录，关键步骤含中文注释。设备/API 检查、数值 compute 与图形渲染采用不同证据；不是每个程序都会弹出窗口。

## 构建和运行

```powershell
cmake --preset dx12
cmake --build --preset dx12-debug --target dx12_queue_submit --parallel 4
.\DirectX12\build\bin\Debug\dx12_queue_submit.exe
```

## 深入学习：数据、接口与执行

- [第 19 课：命令录制、提交与可验证工作](../../chapters/19_record_submit/README.md)

### 本课的真实源码切片

来自 [DirectX12/lessons/dx12_queue_submit/main.cpp](main.cpp#L14)，第 14—37 行；这是现有实现的局部摘录，末尾可能处于函数中间，不是可单独编译的完整程序。

```cpp
int runChapter(int argc, char** argv) {
    auto options = labOptions(argc, argv, 12);
    DxDevice c(options.warp);
    {
        c.begin();
        dxCheck(c.cmd->Close());
        ID3D12CommandList* lists[] = {c.cmd.Get()};
        // 提交是异步的；Fence 完成值决定 allocator 与上传资源何时可复用。
        c.queue->ExecuteCommandLists(1, lists);
        c.wait();
        require(c.fence->GetCompletedValue() >= c.value, "Queue fence did not complete");
        std::cout << "Verified command list close/submit and fence completion\n";
    }
    c.checkMessages();
    return 0;
}
} // namespace course

int main(int argc, char** argv) try {
    return course::runChapter(argc, argv);
} catch (const std::exception& error) {
    std::cerr << "ERROR: " << error.what() << '\n';
    return 1;
}
```

### 同主题跨 API 导航

| 路线 | 程序 |
| --- | --- |
| DirectX 12 | [dx12_queue_submit](README.md) |
| 对照后端 | [vk12_queue_submit](../../../Vulkan/native/lessons/vk12_queue_submit/README.md) |

### 先预测，再验证

分别在录制结束、提交返回和完成等待之后设置 CPU 标记，说明哪一个允许读取结果。

参考推导：只有对应 GPU 完成条件成立且读取依赖正确时允许读取。前两个标记描述 CPU 进度，不证明 GPU 已写完。

## 验证与边界

运行 `--smoke` 执行有限自动检查。数值实验读取实际 GPU 结果，图形实验检查非空图像并可查看读回。交互重建与性能还需额外证据；窗口 renderer 当前保留串行等待，双槽计算实验不会被描述为完整多帧呈现引擎。
