# DirectX 12 · 54 课完整主线

[路线首页](README.md) · [三 API 对照](../docs/COMPARE.md)

按照指定 Vulkan 教程的基础顺序组织，再加入工程扩展。每课包括目的、接口推导、本章完整源码/着色器/CMake、可运行实验、失败定位和参考答案。每章可独立构建；54 章与 54 个专题实验是两套索引：章号用于学习次序，实验号用于保持已有链接；同一实验可能服务多个相关小节。

| 章 | 主题 | 验收关注 |
| --- | --- | --- |
| 01 | [图形 API 的职责与课程地图](chapters/01_architecture/README.md) | 画出一个三角形所需的五类对象，并说明哪些对象保存字节、哪些只保存解释或引用。 |
| 02 | [工具链、SDK、依赖与可复现构建](chapters/02_environment/README.md) | 故意在副本中改变运行工作目录，再从命令行启动程序，说明为何它仍应能找到 shader/assets。 |
| 03 | [应用结构、RAII 与 GPU 使用期](chapters/03_lifetime/README.md) | 将临时 upload 的回收点标到时间图上；指出“Execute/Submit 返回”之后还缺哪条证据。 |
| 04 | [实例扩展与 DXGI 工厂](chapters/04_instance_factory/README.md) | 把某个窗口相关要求从无窗口计算路径中移除，先说明哪些操作因此不再允许，再解释为什么计算仍能成立。 |
| 05 | [验证层、调试队列与诊断证据](chapters/05_validation/README.md) | 注入一条 INFO 应用消息，确认它会进入日志/回调而不会增加 API 错误计数。 |
| 06 | [物理设备、Adapter 与格式能力](chapters/06_physical_device/README.md) | 写出本项目基础与 MSAA 实验各自的能力清单，并标明查询位置。 |
| 07 | [队列族、命令队列与并行边界](chapters/07_queues/README.md) | 说明为什么同一 queue 上顺序提交，仍可能需要资源内存屏障。 |
| 08 | [逻辑设备、特性链与基础完成条件](chapters/08_logical_device/README.md) | 在特性查询和启用处分别打断点，记录同一特性的两次值和来源。 |
| 09 | [窗口 Surface 与 HWND 连接](chapters/09_surface/README.md) | 把同一段 buffer compute 放到没有窗口的程序里，说明它与呈现路径共享哪些对象。 |
| 10 | [交换链选择、格式与呈现模式](chapters/10_swapchain/README.md) | 解释为何两帧 CPU 资源槽与三张交换链图像可以同时存在。 |
| 11 | [图像与 View：限定子资源访问](chapters/11_image_views/README.md) | 只把 view 的 mip 范围缩到最后一级，为什么资源的总分配大小没有随之缩小？ |
| 12 | [图形管线的组合与兼容性](chapters/12_pipeline_overview/README.md) | 将片元/像素输出改为固定洋红，写出黑屏排查的最短路径。 |
| 13 | [着色器编译、模块与反射](chapters/13_shader_modules/README.md) | 将 shader 的 local size 改为 32，但 host 的组数不改，预测覆盖范围和验证结果。 |
| 14 | [固定功能、颜色掩码、视口与剔除](chapters/14_fixed_function/README.md) | 先预测半视口与半裁剪的区别，再分别运行对应实验，检查模型宽高和可见区域。 |
| 15 | [Render Pass、动态渲染与附件契约](chapters/15_rendering_scope/README.md) | 若下一个 pass 要采样本遍颜色，指出除“保存颜色”以外还需什么。 |
| 16 | [Pipeline Layout、根签名与小常量](chapters/16_pipeline_interface/README.md) | 把 factor/bias 改为 9/17，如何证明 GPU 确实收到了新常量？ |
| 17 | [帧缓冲、RTV/DSV 与尺寸相关资源](chapters/17_targets/README.md) | 列出 resize 影响的对象，并区分需要重建和只需更新参数的内容。 |
| 18 | [命令池、Allocator 与安全重置](chapters/18_command_storage/README.md) | 四次录制分别写 41、42、43、44，怎样验证重置与执行链路？ |
| 19 | [命令录制、提交与可验证工作](chapters/19_record_submit/README.md) | 分别在录制结束、提交返回和完成等待之后设置 CPU 标记，说明哪一个允许读取结果。 |
| 20 | [获取、提交、呈现与完成标记](chapters/20_present_sync/README.md) | 解释为何 Vulkan 在 reset 后遇到 OUT_OF_DATE 并直接 continue，可能让下一帧永久等待。 |
| 21 | [两帧资源环与在途使用期](chapters/21_frames_in_flight/README.md) | 把槽数改为三，列出所有必须一起改变的数组和完成条件。 |
| 22 | [窗口变化与交换链重建](chapters/22_swapchain_recreation/README.md) | 普通交互模式下反复拖动、最小化、恢复窗口，记录会触发哪些重建路径。 |
| 23 | [顶点语义、Stride 与 Offset](chapters/23_vertex_input/README.md) | 把某一个属性偏移故意改错，先预测影响的是形状、颜色还是 UV，再用 mesh viewer 查字节。 |
| 24 | [创建顶点资源与上传最小路径](chapters/24_vertex_buffer/README.md) | 记录顶点数、stride、总字节数与 draw 参数，并指出哪些可以直接比较、哪些单位不同。 |
| 25 | [内存类型、Heap 与分配要求](chapters/25_memory/README.md) | 比较请求 257 字节、返回的 allocation size、常量槽 stride 三者，解释不同来源。 |
| 26 | [暂存上传、设备资源与读回闭环](chapters/26_staging/README.md) | 若最终只检查一个元素，哪些错误可能漏掉？ |
| 27 | [索引绘制与顶点复用边界](chapters/27_indices/README.md) | 合并两个 mesh 的顶点和索引，写出第二个 mesh 索引的修正规则。 |
| 28 | [常量结构、矩阵存储与对齐](chapters/28_uniform_layout/README.md) | 在 CPU 打印 sizeof/offsetof，再通过 GPU 计算验证第三个常量槽的 factor/bias。 |
| 29 | [描述符布局与资源接口声明](chapters/29_descriptor_layout/README.md) | 为一个每帧相机、每材质纹理、每对象 model 的场景设计更新频率划分。 |
| 30 | [描述符池、Heap 与资源引用更新](chapters/30_descriptor_instances/README.md) | 修改绑定到 B 后，为什么还要检查 A？ |
| 31 | [动态 UBO 与 Root CBV 地址选择](chapters/31_dynamic_offsets/README.md) | 把三个槽的参数互换而不改变 shader，预测结果应如何变化。 |
| 32 | [Push/Root Constants 的单位与边界](chapters/32_per_draw_constants/README.md) | 加入第二个对象，仅改变它的 model，说明不需要重建哪些长期对象。 |
| 33 | [真实图片、通道与资产路径](chapters/33_image_decode/README.md) | 把图片换成带透明通道的四象限测试图，先检查方向与字节数，再讨论混合。 |
| 34 | [图像存储、Footprint 与拷贝](chapters/34_image_upload/README.md) | 解释一次 memcpy(width*height*4) 为什么不总能正确填 texture upload。 |
| 35 | [图像布局、资源状态与读写依赖](chapters/35_image_dependencies/README.md) | 把纹理消费者从 pixel/fragment 改为 compute，哪些同步描述也要一起改变？ |
| 36 | [采样器、过滤、寻址与 UV](chapters/36_samplers/README.md) | 在 UV=0..4 的测试几何上比较 clamp 与 repeat，解释边缘拉伸来自哪里。 |
| 37 | [图像、采样器与 Shader 资源连接](chapters/37_texture_binding/README.md) | 为什么 descriptor/SRV 看起来正确，纹理仍然可能全黑？ |
| 38 | [sRGB、线性光照与显示编码](chapters/38_color_space/README.md) | 给粗糙度贴图使用 sRGB 读取，能否靠 normalize 或调曝光恢复正确粗糙度？ |
| 39 | [深度格式、用途与样本支持](chapters/39_depth_formats/README.md) | 为颜色和深度选择共同支持的样本数，写出如何取合法交集。 |
| 40 | [遮挡、清除、比较与写入](chapters/40_depth_test/README.md) | 固定几何和相机，只把 clear 改为零而继续 LESS，预测结果。 |
| 41 | [OBJ 导入、索引与几何归一化](chapters/41_obj_models/README.md) | 只加载模型位置并用固定色画出，再分阶段恢复 normal/UV，说明这种顺序的作用。 |
| 42 | [glTF 节点层级、法线与静态姿态](chapters/42_gltf_nodes/README.md) | 给 glTF 的父节点增加平移，预测所有后代是否一起移动，并说明法线如何处理非均匀缩放。 |
| 43 | [GPU Mip 链生成与逐级验收](chapters/43_mip_generation/README.md) | 把基图改为棋盘或分区整数颜色，为什么比任意照片更适合作为 mip 数值验收？ |
| 44 | [LOD、子资源视图与三线性过滤](chapters/44_mip_sampling/README.md) | 固定相机观察远处小物体，比较单级、完整 mip 与末级 view，并记录哪些差异来自范围、哪些来自过滤。 |
| 45 | [多重采样、匹配附件与 Resolve](chapters/45_msaa/README.md) | 在斜边放大观察单样本与 4x，同时说明为什么纹理莫尔纹仍可能存在。 |
| 46 | [计算管线、线程组与全局索引](chapters/46_compute/README.md) | 为什么 shader 用组内索引而不是全局索引，会让四组写到同一段数据？ |
| 47 | [Storage/UAV、边界与 257 项实验](chapters/47_storage_tail/README.md) | 只把 CPU 验证改成 257 而仍分配 256 项，会产生什么问题？ |
| 48 | [计算生成顶点并直接绘制](chapters/48_compute_graphics/README.md) | 把粒子数改为 97，需要检查哪些容量与 dispatch/guard？ |
| 49 | [组共享内存、归约与屏障一致性](chapters/49_group_shared/README.md) | 若尾部 lane 在第一个 barrier 前直接 return，为什么不是一种安全优化？ |
| 50 | [Timeline、Fence 计数与生产消费链](chapters/50_timeline/README.md) | 说明 host 等 counter=2 与 GPU queue 等 counter=1 的区别。 |
| 51 | [间接参数、命令签名与可扩展提交](chapters/51_indirect/README.md) | 把 vertex count 改为零，预测为何应该没有三角形，并说明它与缓冲为空的区别。 |
| 52 | [相机、法线与直接光 PBR 对齐](chapters/52_camera_material/README.md) | 先输出 N，再输出 NoL，最后加入 D/G/F，说明这样如何定位亮度不一致。 |
| 53 | [GPU 时间戳、管线缓存与测量边界](chapters/53_measure_cache/README.md) | 列出一个公平跨 API 性能实验至少需要固定的六项条件。 |
| 54 | [综合场景、验收与工程化边界](chapters/54_capstone/README.md) | 选择阴影、IBL 或骨骼动画之一，先写资源表、pass 依赖图、中间调试视图和验收条件，再实现。 |
