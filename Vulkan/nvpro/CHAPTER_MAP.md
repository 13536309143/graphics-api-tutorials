# nvpro_core2 · 原生 Vulkan 54 课框架对照

[框架路线首页](README.md) · [原生 54 课](../native/COURSE.md)

按每章列出 Context、Allocator、Application、RenderTarget 与自定义 pass 的边界。保留现有六个可运行框架示例；未接入的高级功能明确作为移植工作。

| 章 / 原生教材 | 框架职责与边界 |
| --- | --- |
| [01 图形 API 的职责与课程地图](../native/chapters/01_architecture/README.md) | 把 nvvk::Context 对应到 instance/device/queue，把 nvapp::Application 对应到平台与帧循环；自定义 SceneVk 仍负责实际绘制。 |
| [02 工具链、SDK、依赖与可复现构建](../native/chapters/02_environment/README.md) | 固定子模块提交和配套 Vulkan headers/Volk，按依赖文档准备。框架主分支的最新接口与本项目固定版本可能不同，不直接混接。 |
| [03 应用结构、RAII 与 GPU 使用期](../native/chapters/03_lifetime/README.md) | ResourceAllocator 的 destroyBuffer 仍要求调用方确保无在途引用。onDetach 应先等待 GPU，再释放依赖 allocator 的对象，最后 deinit allocator。 |
| [04 实例扩展与 DXGI 工厂](../native/chapters/04_instance_factory/README.md) | ContextInitInfo 的 instanceExtensions 与 deviceExtensions 分开填写；框架自动创建不代表应用可以忽略扩展所属层次。 |
| [05 验证层、调试队列与诊断证据](../native/chapters/05_validation/README.md) | Context 可启用 validation，但自定义资源的错误仍需由应用回调和日志检查。抓 nvapp 帧时区分场景 pass 与最后 UI pass。 |
| [06 物理设备、Adapter 与格式能力](../native/chapters/06_physical_device/README.md) | Context 的设备选择结果应与需求列表一起审查。框架选择到一个可用 device 后，新的效果仍可能要求额外格式或 sample count。 |
| [07 队列族、命令队列与并行边界](../native/chapters/07_queues/README.md) | ContextInitInfo 中声明队列需求，getQueueInfo 返回 queue 与 familyIndex。自定义 helper 要使用对应 family 创建 pool，而不是默认所有 GPU 都是 family 0。 |
| [08 逻辑设备、特性链与基础完成条件](../native/chapters/08_logical_device/README.md) | Context 汇总这些创建步骤。阅读框架时关注需求到启用链的转换及错误处理，不要求逐次抄写所有结构体。 |
| [09 窗口 Surface 与 HWND 连接](../native/chapters/09_surface/README.md) | 非 headless 的 Application 需要 surface/swapchain 扩展；headless 目标仍可使用 RenderTarget 与图像读回，无需 UI。 |
| [10 交换链选择、格式与呈现模式](../native/chapters/10_swapchain/README.md) | Application 管理交换链，用户 viewport 是另一组离屏目标。比较尺寸时区分外层窗口与 Viewport 实际像素区域。 |
| [11 图像与 View：限定子资源访问](../native/chapters/11_image_views/README.md) | RenderTarget 产生图像和 view，ImTexture 引用供 UI 显示的 view。重建 target 后需要更新引用；旧 view 不会自动指向新分配。 |
| [12 图形管线的组合与兼容性](../native/chapters/12_pipeline_overview/README.md) | 框架提供命令与目标，不替你决定 SceneVk 的 shader、PSO 组合和格式兼容。onRender 仍需正确绑定和 draw。 |
| [13 着色器编译、模块与反射](../native/chapters/13_shader_modules/README.md) | 使用项目固定 glslc 与路径，shader 与框架语言选择分开。最新官方 Slang 代码不能作为当前 GLSL 模块的直接输入。 |
| [14 固定功能、颜色掩码、视口与剔除](../native/chapters/14_fixed_function/README.md) | 同样由 SceneVk 处理这些状态。把目标放进 ImGui 并不会修复场景管线的 scissor 或 front-face 错误。 |
| [15 Render Pass、动态渲染与附件契约](../native/chapters/15_rendering_scope/README.md) | RenderTargetState 可以填附件并开始 dynamic rendering，但调用者仍定义附件用途及后续读依赖。GENERAL layout 不代表无需同步。 |
| [16 Pipeline Layout、根签名与小常量](../native/chapters/16_pipeline_interface/README.md) | Framework 不改变 pipeline layout 的二进制约定。nvpro 视口复用相同 SceneVk，学习时沿实际 push 更新进入 shader。 |
| [17 帧缓冲、RTV/DSV 与尺寸相关资源](../native/chapters/17_targets/README.md) | RenderTarget::update 负责尺寸相关分配，之后 ImTexture 更新 view。新 view 的引用与旧 GPU 工作必须分别处理。 |
| [18 命令池、Allocator 与安全重置](../native/chapters/18_command_storage/README.md) | Application 管理自己的命令，临时上传命令有另外的提交/等待接口。不要在自定义代码中随意 reset 框架正在使用的 pool/list。 |
| [19 命令录制、提交与可验证工作](../native/chapters/19_record_submit/README.md) | createTempCmdBuffer 与 submitAndWaitTempCmdBuffer 适合初始化阶段的有限上传；循环中反复等待会牺牲重叠，应在掌握资源使用期后再优化。 |
| [20 获取、提交、呈现与完成标记](../native/chapters/20_present_sync/README.md) | Application 负责其呈现链路，但自定义 UBO/离屏目标需要明确复用规则。框架的完成对象与用户资源生命周期要一起检查。 |
| [21 两帧资源环与在途使用期](../native/chapters/21_frames_in_flight/README.md) | 框架有自己的帧循环，但本课程 SceneVk 仍共用一份 UBO 并等待队列。不能因为 Application 支持调度就认为自定义资源已自动变成多帧安全。 |
| [22 窗口变化与交换链重建](../native/chapters/22_swapchain_recreation/README.md) | onResize 与 RenderTarget::update 提供明确接入点。target 变化后更新 ImTexture；若使用自己的缓存图像，也要给它同样的重建契约。 |
| [23 顶点语义、Stride 与 Offset](../native/chapters/23_vertex_input/README.md) | VMA 改变分配方式，不改变 Vertex 的字节布局。把 GPU storage buffer 同时当顶点缓冲时，更需要核对两种接口的 stride。 |
| [24 创建顶点资源与上传最小路径](../native/chapters/24_vertex_buffer/README.md) | ResourceAllocator/StagingUploader 可以让最终缓冲 device-local。先核对基础例的字节正确，再比较 helper 替代了哪些手动步骤。 |
| [25 内存类型、Heap 与分配要求](../native/chapters/25_memory/README.md) | VMA 从重复分配逻辑中抽象出 allocator，但 flags、访问目标与释放时机仍由调用方决定。不要把 allocator 当成无条件允许 Map 的通行证。 |
| [26 暂存上传、设备资源与读回闭环](../native/chapters/26_staging/README.md) | StagingUploader 的 append 与 cmdUploadAppended 对应准备和录制；submitAndWait 后才可以 deinit 临时上传器。框架不使 CPU 数据地址永久有效。 |
| [27 索引绘制与顶点复用边界](../native/chapters/27_indices/README.md) | 共用同一 Geometry/SceneVk，框架分配并不改变索引意义。模型合并时要把局部索引加上新增顶点的 base。 |
| [28 常量结构、矩阵存储与对齐](../native/chapters/28_uniform_layout/README.md) | 共享 Frame/Draw 与相同 shader，没有另一种框架专用矩阵规则。增加骨骼或其他常量时仍需新的布局与容量验证。 |
| [29 描述符布局与资源接口声明](../native/chapters/29_descriptor_layout/README.md) | SceneVk 使用原生 layout/set；Context 与 Application 不自动推导自定义 shader 的资源接口。 |
| [30 描述符池、Heap 与资源引用更新](../native/chapters/30_descriptor_instances/README.md) | VMA 管理资源内存，不管理所有自定义 descriptor 更新。即使 helper 分配了 set，pool、set 和被引用资源仍有各自生命周期。 |
| [31 动态 UBO 与 Root CBV 地址选择](../native/chapters/31_dynamic_offsets/README.md) | 框架路径仍可使用原生 dynamic descriptor；不要假定 allocator 已自动把任意应用偏移变成合法对齐。 |
| [32 Push/Root Constants 的单位与边界](../native/chapters/32_per_draw_constants/README.md) | SceneVk 与 nvpro 使用相同 push 布局；如果换自定义 shader，需要自己维护接口兼容，不由 UI 参数自动生成。 |
| [33 真实图片、通道与资产路径](../native/chapters/33_image_decode/README.md) | 框架可提供额外资产工具，但本教程的原生图片路径独立于框架；对照时分开查看解码和 GPU 上传两层。 |
| [34 图像存储、Footprint 与拷贝](../native/chapters/34_image_upload/README.md) | StagingUploader 简化收集和拷贝录制，但输入数据、用途与完成回收仍要解释。当前 nv04 的 VMA 演示重点是顶点，不能写成已迁移全部图片管线。 |
| [35 图像布局、资源状态与读写依赖](../native/chapters/35_image_dependencies/README.md) | GENERAL→GENERAL 也可能需要依赖。RenderTarget 与 ImGui 的颜色写后采样正是布局不变而访问变化的例子。 |
| [36 采样器、过滤、寻址与 UV](../native/chapters/36_samplers/README.md) | 同样受 Vulkan sampler 规则约束。ImGui 显示采样与场景材质采样是不同用途，不能把 UI 的设置当成场景 sampler 已更新。 |
| [37 图像、采样器与 Shader 资源连接](../native/chapters/37_texture_binding/README.md) | SceneVk 管理自定义材质 descriptor，ImTexture 管理 UI 显示引用。两套绑定服务不同 shader，不因指向同一图像就可以任意替换。 |
| [38 sRGB、线性光照与显示编码](../native/chapters/38_color_space/README.md) | 框架的 UI 显示与场景输出可能各有编码假设。比对时先固定目标格式和最终编码，避免场景先 gamma、UI 又 gamma。 |
| [39 深度格式、用途与样本支持](../native/chapters/39_depth_formats/README.md) | RenderTarget::CreateInfo 中 depthFormat 也是明确需求。框架替你分配附件，不代表任意格式和 sample count 组合都成立。 |
| [40 遮挡、清除、比较与写入](../native/chapters/40_depth_test/README.md) | 视口 target 的 depth 与颜色尺寸共同变化。UI 能显示纹理不证明场景内部深度状态正确，应查看离屏 depth 或对应事件。 |
| [41 OBJ 导入、索引与几何归一化](../native/chapters/41_obj_models/README.md) | 资产解析、VMA 上传、descriptor 和 scene hierarchy 应分别设计。本轮框架路线保留六个基础例，提供接入点对照，不把原生 Assimp 导入算成已完成的 nvpro 模型播放器。 |
| [42 glTF 节点层级、法线与静态姿态](../native/chapters/42_gltf_nodes/README.md) | 可将共享 CPU 层接到 ResourceAllocator/StagingUploader，但还需按 node/material 组织 GPU 对象和 draw。不要直接把 OpenGL ImportedMesh 类换名前缀。 |
| [43 GPU Mip 链生成与逐级验收](../native/chapters/43_mip_generation/README.md) | 框架可能提供更多图像辅助，但应用必须确定支持格式、子资源范围与生成依赖。native mip helper 的规则可直接用于对照分析。 |
| [44 LOD、子资源视图与三线性过滤](../native/chapters/44_mip_sampling/README.md) | ImGui 展示纹理时的采样方式不等于三维场景的导数和 LOD；要在 SceneVk 材质 draw 中检查实际 shader 读取。 |
| [45 多重采样、匹配附件与 Resolve](../native/chapters/45_msaa/README.md) | RenderTarget 的样本管理需与自定义 pipeline 和最终 UI 可采样目标匹配。框架对照着重列出这些依赖，不声称现有 nv03 默认已经开启 4x。 |
| [46 计算管线、线程组与全局索引](../native/chapters/46_compute/README.md) | nv05 由 Context 提供设备/队列并复用 computeSquares。它不创建普通窗口循环，成功结果在运行输出中查看。 |
| [47 Storage/UAV、边界与 257 项实验](../native/chapters/47_storage_tail/README.md) | Context 不改变边界算法；将相同 count/dispatch 约定带入框架路径即可比较初始化层差异。 |
| [48 计算生成顶点并直接绘制](../native/chapters/48_compute_graphics/README.md) | 框架可提供分配和调度，但必须与自定义 vertex storage 的使用期协调。不能只把 compute 插入 onRender 而不核对命令与资源复用。 |
| [49 组共享内存、归约与屏障一致性](../native/chapters/49_group_shared/README.md) | 框架不改变工作组执行语义；使用原生或 nvpro 都要满足相同的一致控制流要求。 |
| [50 Timeline、Fence 计数与生产消费链](../native/chapters/50_timeline/README.md) | Context 特性需求需要包含所用 timeline 能力；Application 自身的帧调度与用户完成值应有明确边界。 |
| [51 间接参数、命令签名与可扩展提交](../native/chapters/51_indirect/README.md) | 相同 SceneVk 可以接入 indirectArguments，框架仍负责外围目标/帧循环。不能只创建一个 indirect buffer 而继续走普通 Draw 就算完成该特性。 |
| [52 相机、法线与直接光 PBR 对齐](../native/chapters/52_camera_material/README.md) | nv06 的 roughness UI 与原生 PBR 共享 shader，但视口尺寸/UI 成本不同。对照实验先固定倍率和目标大小。 |
| [53 GPU 时间戳、管线缓存与测量边界](../native/chapters/53_measure_cache/README.md) | UI、离屏与框架调度应从场景 pass 测量中区分。工具抓帧的额外开销也不等于发布版正常帧成本。 |
| [54 综合场景、验收与工程化边界](../native/chapters/54_capstone/README.md) | 作为逐课框架对照，给出 Context/Allocator/Application/RenderTarget 接入责任；六个 nvpro 例仍独立可用。本轮没有将原生 54 个实验重新包装成另一套已完成框架程序。 |
