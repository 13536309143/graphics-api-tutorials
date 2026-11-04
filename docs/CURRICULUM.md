# Vulkan / DX12 · 54 课逐章对照

[Vulkan 主线](../Vulkan/native/COURSE.md) · [DX12 主线](../DirectX12/COURSE.md) · [nvpro 框架对照](../Vulkan/nvpro/CHAPTER_MAP.md)

两条主线各 54 章与 54 个独立可执行实验；章号依官方概念顺序，实验号保留旧入口。框架路线按用户选择作为逐章对照，仍有六个独立框架例子。

| 章 | 主题 | Vulkan | DX12 |
| --- | --- | --- | --- |
| 01 | 图形 API 的职责与课程地图 | [阅读](../Vulkan/native/chapters/01_architecture/README.md) | [阅读](../DirectX12/chapters/01_architecture/README.md) |
| 02 | 工具链、SDK、依赖与可复现构建 | [阅读](../Vulkan/native/chapters/02_environment/README.md) | [阅读](../DirectX12/chapters/02_environment/README.md) |
| 03 | 应用结构、RAII 与 GPU 使用期 | [阅读](../Vulkan/native/chapters/03_lifetime/README.md) | [阅读](../DirectX12/chapters/03_lifetime/README.md) |
| 04 | 实例扩展与 DXGI 工厂 | [阅读](../Vulkan/native/chapters/04_instance_factory/README.md) | [阅读](../DirectX12/chapters/04_instance_factory/README.md) |
| 05 | 验证层、调试队列与诊断证据 | [阅读](../Vulkan/native/chapters/05_validation/README.md) | [阅读](../DirectX12/chapters/05_validation/README.md) |
| 06 | 物理设备、Adapter 与格式能力 | [阅读](../Vulkan/native/chapters/06_physical_device/README.md) | [阅读](../DirectX12/chapters/06_physical_device/README.md) |
| 07 | 队列族、命令队列与并行边界 | [阅读](../Vulkan/native/chapters/07_queues/README.md) | [阅读](../DirectX12/chapters/07_queues/README.md) |
| 08 | 逻辑设备、特性链与基础完成条件 | [阅读](../Vulkan/native/chapters/08_logical_device/README.md) | [阅读](../DirectX12/chapters/08_logical_device/README.md) |
| 09 | 窗口 Surface 与 HWND 连接 | [阅读](../Vulkan/native/chapters/09_surface/README.md) | [阅读](../DirectX12/chapters/09_surface/README.md) |
| 10 | 交换链选择、格式与呈现模式 | [阅读](../Vulkan/native/chapters/10_swapchain/README.md) | [阅读](../DirectX12/chapters/10_swapchain/README.md) |
| 11 | 图像与 View：限定子资源访问 | [阅读](../Vulkan/native/chapters/11_image_views/README.md) | [阅读](../DirectX12/chapters/11_image_views/README.md) |
| 12 | 图形管线的组合与兼容性 | [阅读](../Vulkan/native/chapters/12_pipeline_overview/README.md) | [阅读](../DirectX12/chapters/12_pipeline_overview/README.md) |
| 13 | 着色器编译、模块与反射 | [阅读](../Vulkan/native/chapters/13_shader_modules/README.md) | [阅读](../DirectX12/chapters/13_shader_modules/README.md) |
| 14 | 固定功能、颜色掩码、视口与剔除 | [阅读](../Vulkan/native/chapters/14_fixed_function/README.md) | [阅读](../DirectX12/chapters/14_fixed_function/README.md) |
| 15 | Render Pass、动态渲染与附件契约 | [阅读](../Vulkan/native/chapters/15_rendering_scope/README.md) | [阅读](../DirectX12/chapters/15_rendering_scope/README.md) |
| 16 | Pipeline Layout、根签名与小常量 | [阅读](../Vulkan/native/chapters/16_pipeline_interface/README.md) | [阅读](../DirectX12/chapters/16_pipeline_interface/README.md) |
| 17 | 帧缓冲、RTV/DSV 与尺寸相关资源 | [阅读](../Vulkan/native/chapters/17_targets/README.md) | [阅读](../DirectX12/chapters/17_targets/README.md) |
| 18 | 命令池、Allocator 与安全重置 | [阅读](../Vulkan/native/chapters/18_command_storage/README.md) | [阅读](../DirectX12/chapters/18_command_storage/README.md) |
| 19 | 命令录制、提交与可验证工作 | [阅读](../Vulkan/native/chapters/19_record_submit/README.md) | [阅读](../DirectX12/chapters/19_record_submit/README.md) |
| 20 | 获取、提交、呈现与完成标记 | [阅读](../Vulkan/native/chapters/20_present_sync/README.md) | [阅读](../DirectX12/chapters/20_present_sync/README.md) |
| 21 | 两帧资源环与在途使用期 | [阅读](../Vulkan/native/chapters/21_frames_in_flight/README.md) | [阅读](../DirectX12/chapters/21_frames_in_flight/README.md) |
| 22 | 窗口变化与交换链重建 | [阅读](../Vulkan/native/chapters/22_swapchain_recreation/README.md) | [阅读](../DirectX12/chapters/22_swapchain_recreation/README.md) |
| 23 | 顶点语义、Stride 与 Offset | [阅读](../Vulkan/native/chapters/23_vertex_input/README.md) | [阅读](../DirectX12/chapters/23_vertex_input/README.md) |
| 24 | 创建顶点资源与上传最小路径 | [阅读](../Vulkan/native/chapters/24_vertex_buffer/README.md) | [阅读](../DirectX12/chapters/24_vertex_buffer/README.md) |
| 25 | 内存类型、Heap 与分配要求 | [阅读](../Vulkan/native/chapters/25_memory/README.md) | [阅读](../DirectX12/chapters/25_memory/README.md) |
| 26 | 暂存上传、设备资源与读回闭环 | [阅读](../Vulkan/native/chapters/26_staging/README.md) | [阅读](../DirectX12/chapters/26_staging/README.md) |
| 27 | 索引绘制与顶点复用边界 | [阅读](../Vulkan/native/chapters/27_indices/README.md) | [阅读](../DirectX12/chapters/27_indices/README.md) |
| 28 | 常量结构、矩阵存储与对齐 | [阅读](../Vulkan/native/chapters/28_uniform_layout/README.md) | [阅读](../DirectX12/chapters/28_uniform_layout/README.md) |
| 29 | 描述符布局与资源接口声明 | [阅读](../Vulkan/native/chapters/29_descriptor_layout/README.md) | [阅读](../DirectX12/chapters/29_descriptor_layout/README.md) |
| 30 | 描述符池、Heap 与资源引用更新 | [阅读](../Vulkan/native/chapters/30_descriptor_instances/README.md) | [阅读](../DirectX12/chapters/30_descriptor_instances/README.md) |
| 31 | 动态 UBO 与 Root CBV 地址选择 | [阅读](../Vulkan/native/chapters/31_dynamic_offsets/README.md) | [阅读](../DirectX12/chapters/31_dynamic_offsets/README.md) |
| 32 | Push/Root Constants 的单位与边界 | [阅读](../Vulkan/native/chapters/32_per_draw_constants/README.md) | [阅读](../DirectX12/chapters/32_per_draw_constants/README.md) |
| 33 | 真实图片、通道与资产路径 | [阅读](../Vulkan/native/chapters/33_image_decode/README.md) | [阅读](../DirectX12/chapters/33_image_decode/README.md) |
| 34 | 图像存储、Footprint 与拷贝 | [阅读](../Vulkan/native/chapters/34_image_upload/README.md) | [阅读](../DirectX12/chapters/34_image_upload/README.md) |
| 35 | 图像布局、资源状态与读写依赖 | [阅读](../Vulkan/native/chapters/35_image_dependencies/README.md) | [阅读](../DirectX12/chapters/35_image_dependencies/README.md) |
| 36 | 采样器、过滤、寻址与 UV | [阅读](../Vulkan/native/chapters/36_samplers/README.md) | [阅读](../DirectX12/chapters/36_samplers/README.md) |
| 37 | 图像、采样器与 Shader 资源连接 | [阅读](../Vulkan/native/chapters/37_texture_binding/README.md) | [阅读](../DirectX12/chapters/37_texture_binding/README.md) |
| 38 | sRGB、线性光照与显示编码 | [阅读](../Vulkan/native/chapters/38_color_space/README.md) | [阅读](../DirectX12/chapters/38_color_space/README.md) |
| 39 | 深度格式、用途与样本支持 | [阅读](../Vulkan/native/chapters/39_depth_formats/README.md) | [阅读](../DirectX12/chapters/39_depth_formats/README.md) |
| 40 | 遮挡、清除、比较与写入 | [阅读](../Vulkan/native/chapters/40_depth_test/README.md) | [阅读](../DirectX12/chapters/40_depth_test/README.md) |
| 41 | OBJ 导入、索引与几何归一化 | [阅读](../Vulkan/native/chapters/41_obj_models/README.md) | [阅读](../DirectX12/chapters/41_obj_models/README.md) |
| 42 | glTF 节点层级、法线与静态姿态 | [阅读](../Vulkan/native/chapters/42_gltf_nodes/README.md) | [阅读](../DirectX12/chapters/42_gltf_nodes/README.md) |
| 43 | GPU Mip 链生成与逐级验收 | [阅读](../Vulkan/native/chapters/43_mip_generation/README.md) | [阅读](../DirectX12/chapters/43_mip_generation/README.md) |
| 44 | LOD、子资源视图与三线性过滤 | [阅读](../Vulkan/native/chapters/44_mip_sampling/README.md) | [阅读](../DirectX12/chapters/44_mip_sampling/README.md) |
| 45 | 多重采样、匹配附件与 Resolve | [阅读](../Vulkan/native/chapters/45_msaa/README.md) | [阅读](../DirectX12/chapters/45_msaa/README.md) |
| 46 | 计算管线、线程组与全局索引 | [阅读](../Vulkan/native/chapters/46_compute/README.md) | [阅读](../DirectX12/chapters/46_compute/README.md) |
| 47 | Storage/UAV、边界与 257 项实验 | [阅读](../Vulkan/native/chapters/47_storage_tail/README.md) | [阅读](../DirectX12/chapters/47_storage_tail/README.md) |
| 48 | 计算生成顶点并直接绘制 | [阅读](../Vulkan/native/chapters/48_compute_graphics/README.md) | [阅读](../DirectX12/chapters/48_compute_graphics/README.md) |
| 49 | 组共享内存、归约与屏障一致性 | [阅读](../Vulkan/native/chapters/49_group_shared/README.md) | [阅读](../DirectX12/chapters/49_group_shared/README.md) |
| 50 | Timeline、Fence 计数与生产消费链 | [阅读](../Vulkan/native/chapters/50_timeline/README.md) | [阅读](../DirectX12/chapters/50_timeline/README.md) |
| 51 | 间接参数、命令签名与可扩展提交 | [阅读](../Vulkan/native/chapters/51_indirect/README.md) | [阅读](../DirectX12/chapters/51_indirect/README.md) |
| 52 | 相机、法线与直接光 PBR 对齐 | [阅读](../Vulkan/native/chapters/52_camera_material/README.md) | [阅读](../DirectX12/chapters/52_camera_material/README.md) |
| 53 | GPU 时间戳、管线缓存与测量边界 | [阅读](../Vulkan/native/chapters/53_measure_cache/README.md) | [阅读](../DirectX12/chapters/53_measure_cache/README.md) |
| 54 | 综合场景、验收与工程化边界 | [阅读](../Vulkan/native/chapters/54_capstone/README.md) | [阅读](../DirectX12/chapters/54_capstone/README.md) |
