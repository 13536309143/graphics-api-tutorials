# 第 26 课 · 暂存上传、设备资源与读回闭环 · Vulkan 原生

[54 课目录](../../COURSE.md) · [对照精讲](../../../../docs/compare/03_geometry_memory.md) · [上一课](../25_memory/README.md) · [下一课](../27_indices/README.md)

## 本课目标与先修

学习目标：解释“暂存上传、设备资源与读回闭环”解决的问题，找到实际对象和参数，运行对应实验并用数据/画面证明。先完成前一课，并能说明它交付的对象与完成条件；本课在该依赖上增加新契约。

## 为什么需要这一层

本课用 257 个确定整数做完整闭环：CPU 写 upload，中转到 GPU 设备资源，再复制到 readback，等待完成并逐项核对。奇数长度帮助发现把字节数、元素数或范围截断的错误。

## Vulkan 原生 的对象、参数与执行推导

源/目标分别声明 TRANSFER_SRC/DST，设备缓冲选择 device-local。第一次 copy 的写入通过 buffer barrier 对第二次 transfer read 可见，最后 copy 写入经 host-read 依赖供 CPU 检查。

## 对照 DirectX 12：相同问题的另一种分工

UPLOAD 固定 GENERIC_READ，DEFAULT 先 COPY_DEST，完成第一份 copy 后转 COPY_SOURCE，READBACK 固定 COPY_DEST。不能因为都叫 BufferRegion 就省掉资源状态；临时 upload 在 fence 前保持存活。

## 关键参数与可检查不变量

| 检查项 | Vulkan 表达 | DX12 表达 | 不变量 / 证据 |
| --- | --- | --- | --- |
| 闭环容量 | 257 个 uint32 | 257 个 uint32 | 每项 i*17+9 可独立核对 |
| 中间存储 | device-local buffer | DEFAULT resource | upload/readback 与设备资源分离 |
| 依赖链 | copy-write→copy-read→host-read | COPY_DEST→COPY_SOURCE→readback | 临时 upload 活到完成后 |

先为每项指出源码中的创建/绑定位置，以及证明它成立的实际日志、字节或图像。数值中的设备限制以查询为准；固定实验尺寸不当作所有设备的上限。

## nvpro_core2 框架对照

StagingUploader 的 append 与 cmdUploadAppended 对应准备和录制；submitAndWait 后才可以 deinit 临时上传器。框架不使 CPU 数据地址永久有效。

这部分是原生 Vulkan 各课的框架对照，框架可运行入口仍为原有六课。如果本节需要新接入工作，会按正文说明；不把原生实验当作已完成的框架实现。

## 本章独立工程与关键注释

[main.cpp](main.cpp) · [本地辅助实现](support/) · [本章着色器](shaders/) · [CMakeLists.txt](CMakeLists.txt)

完整源码就在本目录，关键步骤含中文注释。只复用 GLFW/GLM/Assimp 等第三方库，不包含或链接其他章节的课程实现。单独复制本章文件夹也可配置构建；初始化或纯计算章节不需要图形阶段。

```powershell
cmake -S . -B build -A x64
cmake --build build --config Debug --parallel 4
ctest --test-dir build -C Debug --output-on-failure
```

## 运行与实际检查

| 路线 | 实际实验 |
| --- | --- |
| Vulkan 原生 | [vk13_staging_copy](../../lessons/vk13_staging_copy/README.md) |
| DirectX 12 | [dx13_staging_copy](../../../../DirectX12/lessons/dx13_staging_copy/README.md) |

从合集根目录执行；普通图形程序 Esc 退出，设备/计算/诊断程序输出结果后结束。从“开始学习.cmd”进入本页时，表格可直接点击运行。

```powershell
cmake --preset vulkan-native
cmake --build --preset vulkan-native-debug --target vk26_ch_staging --parallel 4
.\Vulkan\native\build\bin\Debug\vk26_ch_staging.exe
ctest --preset vulkan-native-debug -R "^vk26_ch_staging$" --output-on-failure
```

检查三个层次：创建/绑定/提交的接口约束；数据读回或目标画面；资源使用结束后的回收。诊断实验用合法信息或查询证明渠道可用，不故意保留 API 错误。图形 smoke 的像素检查与数值实验的逐项检查不同，具体实现见源码。

## 关键实现：从当前源码定位

来自 [Vulkan/native/chapters/26_staging/main.cpp](main.cpp#L79)，第 79—100 行。下列是当前仓库的局部切片，可能在函数中间结束；完整上下文请打开文件，切片不作为单独可编译程序。

```cpp
int runChapter(int argc, char** argv) {
    auto options = labOptions(argc, argv, 13);
    nativeShaders = options.directory / "shaders";
    validationErrors = 0;
    {
        NativeDevice owner(nullptr, false);
        const auto& c = owner.c;
        {
            stagingCopy(c);
        }
    }
    std::cout << "Validation errors: " << validationErrors << '\n';
    return validationErrors ? 1 : 0;
}
} // namespace course

int main(int argc, char** argv) try {
    return course::runChapter(argc, argv);
} catch (const std::exception& error) {
    std::cerr << "ERROR: " << error.what() << '\n';
    return 1;
}
```

阅读时按“前置对象 → 参数/布局 → 录制/调用 → 完成/可见性 → 释放”五步标注。辅助实现与着色器是本章自己的文件；修改不会影响其他章。

## 受控修改实验

若最终只检查一个元素，哪些错误可能漏掉？

先记录预测，再只改一个因素；保留基线与修改后的输出。不要同时改变 shader、资源范围和同步，这样即使结果变好也无法归因。

### 参考推导与通过标准

尾部未拷贝、偏移偏一、局部范围错误仍可能让首元素正确。闭环应核对全部元素和容量，再把路径用于顶点或纹理。

## 常见失败与排查顺序

上传后立刻释放/覆盖 upload，或用错误的 consumer access/stage，可能在轻负载下偶然正确。

先看第一条错误和当前执行路径，再看实际数据/绑定，最后看 shader 与画面。需要 GPU 证据时抓取正常交互帧；仅四帧的 smoke 适合自动检查，不适合手动抓帧。

## 自测与本课交付

能否不看源码回答：若最终只检查一个元素，哪些错误可能漏掉？

提交一份简短记录：输入与假设、唯一修改、预期、实际证据、原因、是否恢复基线。如果只能复述函数名而说不清何时允许访问/释放，先回到本课的数据流。

## 官方阅读位置与当前工程差异

[指定 Vulkan 教程对应位置](https://docs.vulkan.net.cn/tutorial/latest/04_Vertex_buffers/02_Staging_buffer.html)。DX12 路线按同一概念顺序建立映射，其对象与约束以 Microsoft 文档和本后端代码为准。49—54 课包含在基础/compute 之后加入的工程扩展。

[版本与资料说明](../../../../docs/REFERENCES.md) · [调试](../../../../docs/DEBUGGING.md) · [功能完成度](../../../../docs/FEATURE_MATRIX.md)
