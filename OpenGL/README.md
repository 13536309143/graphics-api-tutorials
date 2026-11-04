# OpenGL 中文实战教程 · C++17 + CMake

这是一个本地可读、可编译、可修改的学习工程。以 **OpenGL 3.3 Core** 为基线，从第一个窗口到真实模型、三维相机、阴影、PBR/IBL、HDR、延迟渲染和骨骼动画。共 34 个独立可执行示例；每课有中文讲解、观察目标、练习和参考答案。无需预先理解图形学，但应先会 C++ 变量、函数、数组、类和基本指针。

## 现在就开始

双击合集根目录“开始学习.cmd”，选择 OpenGL 章节并点击运行。编译从**合集根目录**执行：

```powershell
cmake --preset opengl
cmake --build --preset opengl-debug --parallel 4
.\OpenGL\build\bin\Debug\02_triangle.exe
```

也可执行 `powershell -ExecutionPolicy Bypass -File .\scripts\build.ps1 -Track opengl -Test`。旧的 OpenGL/scripts/build.ps1 已转发到合集脚本；OpenGL/scripts/run.ps1 仍可在 OpenGL 目录内使用。

首次配置下载固定依赖，之后可使用缓存离线构建。独立配置 OpenGL 子工程时请另用 build-local，不要覆盖根 preset 创建的缓存。各课运行/增量构建命令以 OpenGL 目录为当前目录时仍然适用。

## 按顺序学习

| 课次 | 内容 | CMake target / 运行参数 |
| --- | --- | --- |
| 01 | [窗口与渲染循环](lessons/01_window/README.md) | `01_window` |
| 02 | [第一个彩色三角形](lessons/02_triangle/README.md) | `02_triangle` |
| 03 | [索引绘制与线框](lessons/03_indexed/README.md) | `03_indexed` |
| 04 | [Shader 与 Uniform 动画](lessons/04_uniform/README.md) | `04_uniform` |
| 05 | [纹理与采样](lessons/05_texture/README.md) | `05_texture` |
| 06 | [二维变换](lessons/06_transform/README.md) | `06_transform` |
| 07 | [三维坐标与深度测试](lessons/07_cube/README.md) | `07_cube` |
| 08 | [自由相机](lessons/08_camera/README.md) | `08_camera` |
| 09 | [Phong 光照](lessons/09_lighting/README.md) | `09_lighting` |
| 10 | [材质与多光源](lessons/10_materials/README.md) | `10_materials` |
| 11 | [离屏渲染与后处理](lessons/11_framebuffer/README.md) | `11_framebuffer` |
| 12 | [实例化绘制](lessons/12_instancing/README.md) | `12_instancing` |
| 13 | [阴影贴图](lessons/13_shadows/README.md) | `13_shadows` |

## 进阶课程（14–34）

| 课次 | 内容 | CMake target / 运行参数 |
| --- | --- | --- |
| 14 | [Assimp 与 OBJ 模型](lessons/14_model_obj/README.md) | `14_model_obj` |
| 15 | [glTF 场景与 PBR 材质](lessons/15_model_gltf/README.md) | `15_model_gltf` |
| 16 | [透明混合与排序](lessons/16_blending/README.md) | `16_blending` |
| 17 | [模板测试与描边](lessons/17_stencil/README.md) | `17_stencil` |
| 18 | [天空盒与环境反射](lessons/18_cubemap/README.md) | `18_cubemap` |
| 19 | [法线贴图与 TBN](lessons/19_normal_mapping/README.md) | `19_normal_mapping` |
| 20 | [视差遮蔽映射](lessons/20_parallax/README.md) | `20_parallax` |
| 21 | [线性颜色与 HDR](lessons/21_hdr/README.md) | `21_hdr` |
| 22 | [Bloom 与高斯模糊](lessons/22_bloom/README.md) | `22_bloom` |
| 23 | [Cook-Torrance PBR](lessons/23_pbr/README.md) | `23_pbr` |
| 24 | [PBR 贴图工作流](lessons/24_pbr_textures/README.md) | `24_pbr_textures` |
| 25 | [IBL 与 Split Sum](lessons/25_ibl/README.md) | `25_ibl` |
| 26 | [延迟渲染与 G-buffer](lessons/26_deferred/README.md) | `26_deferred` |
| 27 | [SSAO 与降噪](lessons/27_ssao/README.md) | `27_ssao` |
| 28 | [UBO 与 std140](lessons/28_ubo/README.md) | `28_ubo` |
| 29 | [离屏 MSAA 与 Resolve](lessons/29_msaa/README.md) | `29_msaa` |
| 30 | [几何着色器](lessons/30_geometry/README.md) | `30_geometry` |
| 31 | [骨骼动画与蒙皮](lessons/31_skeletal/README.md) | `31_skeletal` |
| 32 | [实例化粒子](lessons/32_particles/README.md) | `32_particles` |
| 33 | [点光源全方向阴影](lessons/33_point_shadows/README.md) | `33_point_shadows` |
| 34 | [综合实战：PBR 模型展厅](lessons/34_gallery/README.md) | `34_gallery` |

建议每次学习一课：先运行观察，读讲解和 main.cpp，再修改练习，最后回答自测。基础阶段 01–08，光照与渲染入门 09–13，模型与表面 14–20，HDR/PBR/IBL 21–25，管线与进阶效果 26–33，综合实战 34。不要只复制代码：能画出数据从 CPU 到片段输出的路径，才算理解。

## 配套说明

- [环境和构建](docs/01_环境与构建.md)
- [C++ 必备知识](docs/02_C++预备知识.md)
- [CMake 入门](docs/03_CMake入门.md)
- [坐标、向量和矩阵](docs/04_数学与管线.md)
- [OpenGL 状态与资源管理](docs/05_状态与资源.md)
- [排错手册](docs/06_排错手册.md)
- [综合练习与验收](docs/07_综合练习.md)
- [进阶路线和权威参考](docs/08_进阶路线.md)
- [本机验证记录](docs/验证记录.md)
- [模型导入与替换指南](docs/09_模型导入与替换.md)
- [PBR/IBL 推导与管线](docs/10_PBR与IBL.md)
- [进阶排错与性能](docs/11_进阶排错与性能.md)
- [后续专题与实现边界](docs/12_后续专题与边界.md)
- [扩充验证记录](docs/扩充验证记录.md)
- [第三方版本与许可证](THIRD_PARTY.md)

## 操作与文件

Esc 退出；08–13 中含相机的示例使用 W/A/S/D 移动、Q/E 升降、按住鼠标右键转向。03 按空格看线框，11 按空格看彩色，13 按空格暂时关闭阴影。进阶各课的空格用于临时关闭当前效果，21/22 支持上下箭头调曝光，27 支持 1 查看 AO、2 查看法线。详见各课说明。

lessons/ 每课一个 main.cpp 与 README；common/ 放窗口、shader、网格和相机工具；shaders/ 是可编辑 GLSL；scripts/ 放构建运行脚本；assets/ 是模型/纹理/HDR 素材；third_party/ 是固定版本依赖；build/ 是可重新生成的构建产物。普通开发只需要改 lessons、common、shaders。

## 范围

教程已覆盖从入门到模型加载、PBR/IBL、延迟渲染、SSAO、骨骼动画等进阶内容。随附自制 OBJ/glTF、PNG 材质和 HDR 环境素材，开箱即可运行。它仍不是所有图形学主题的百科，也不是完整游戏引擎；尚未实现的主题在后续路线明确列出。OpenGL 驱动来自显卡厂商，不是通过安装某个“OpenGL SDK”升级。运行需要支持 3.3 Core 的图形驱动。


合集部署推荐从仓库根目录运行 `cmake --preset opengl`、`cmake --build --preset opengl-debug`。历史验证记录保留原 34 课开发过程；当前合集的验收以 [总验证记录](../docs/VALIDATION.md) 为准。
