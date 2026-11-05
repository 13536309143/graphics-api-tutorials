# dx13_staging_copy · 设备内存上传与读回

[54 课目录](../../COURSE.md)

## 本课独立工程与关键注释

[main.cpp](main.cpp) · [本地辅助实现](support/) · [本课着色器](shaders/) · [CMakeLists.txt](CMakeLists.txt)

本目录包含独立课程源码，关键位置有中文注释。单独复制整个文件夹后可用 `cmake -S . -B build -A x64` 配置、`cmake --build build --config Debug` 构建；不需要其他章节源码。

## 目的与实际执行

UPLOAD 固定 GENERIC_READ，DEFAULT 先 COPY_DEST，完成第一份 copy 后转 COPY_SOURCE，READBACK 固定 COPY_DEST。不能因为都叫 BufferRegion 就省掉资源状态；临时 upload 在 fence 前保持存活。

GetCopyableFootprints 返回 Offset、RowPitch 和各行信息，逐行填充后录制 CopyTextureRegion。texture resource 的宽高不是 upload buffer 的紧密内存布局；mip/array 各自是子资源。

legacy transition barrier 表达 COPY_DEST、UAV、shader resource 等用途转换。compute mip 每级有自己的 subresource state；UAV 写后下级采样的依赖由对应转换建立。跨队列还需 fence Signal/Wait。

本程序的 C++、辅助实现、着色器和构建脚本均位于本目录，关键步骤含中文注释。设备/API 检查、数值 compute 与图形渲染采用不同证据；不是每个程序都会弹出窗口。

## 构建和运行

```powershell
cmake --preset dx12
cmake --build --preset dx12-debug --target dx13_staging_copy --parallel 4
.\DirectX12\build\bin\Debug\dx13_staging_copy.exe
```

## 深入学习：数据、接口与执行

- [第 26 课：暂存上传、设备资源与读回闭环](../../chapters/26_staging/README.md)
- [第 34 课：图像存储、Footprint 与拷贝](../../chapters/34_image_upload/README.md)
- [第 35 课：图像布局、资源状态与读写依赖](../../chapters/35_image_dependencies/README.md)

### 本课的真实源码切片

来自 [DirectX12/lessons/dx13_staging_copy/main.cpp](main.cpp#L53)，第 53—69 行；这是现有实现的局部摘录，末尾可能处于函数中间，不是可单独编译的完整程序。

```cpp
int runChapter(int argc, char** argv) {
    auto options = labOptions(argc, argv, 13);
    DxDevice c(options.warp);
    {
        stagingCopy(c);
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
| DirectX 12 | [dx13_staging_copy](README.md) |
| 对照后端 | [vk13_staging_copy](../../../Vulkan/native/lessons/vk13_staging_copy/README.md) |

### 先预测，再验证

若最终只检查一个元素，哪些错误可能漏掉？

参考推导：尾部未拷贝、偏移偏一、局部范围错误仍可能让首元素正确。闭环应核对全部元素和容量，再把路径用于顶点或纹理。

解释一次 memcpy(width*height*4) 为什么不总能正确填 texture upload。

参考推导：API 可能要求更大的对齐行距或子资源起点。正确内容应逐行写到 footprint 的 Offset+y*RowPitch，再按有效宽度复制。

把纹理消费者从 pixel/fragment 改为 compute，哪些同步描述也要一起改变？

参考推导：消费者阶段/访问或 state 应反映 compute 读取。直接复用只针对片元的依赖不一定覆盖新的使用者。

## 验证与边界

运行 `--smoke` 执行有限自动检查。数值实验读取实际 GPU 结果，图形实验检查非空图像并可查看读回。交互重建与性能还需额外证据；窗口 renderer 当前保留串行等待，双槽计算实验不会被描述为完整多帧呈现引擎。
