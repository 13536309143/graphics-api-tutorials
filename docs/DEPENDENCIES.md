# 依赖版本与许可证

| 项目 | 固定版本 | 获取方式 |
| --- | --- | --- |
| GLFW | 3.4 | CMake URL + SHA256，或 OpenGL/third_party 本地缓存 |
| GLM | 1.0.1 | 同上 |
| Assimp | 5.4.3 | OpenGL CMake URL + SHA256，仅开启 OBJ/GLTF |
| GLAD | generator 2.0.8 / GL3.3 Core | 已生成源码随 OpenGL 提交 |
| stb_image | d14d46f6b44456eb88beac58cba833a4135d4c89 | 随 OpenGL/third_party/stb 提交 |
| nvpro_core2 | 71ab4212aa3dd8e3b6cb659ed691b59062dc5f11 | Git 子模块 |
| Vulkan-Headers / Utility-Libraries | vulkan-sdk-1.4.341.0 | 项目局部 FetchContent，固定 SHA256 |

Vulkan loader/import library、glslc 和系统 validation layer 来自安装的 SDK/驱动，不提交仓库。nvpro 的 Volk 采用同一子模块版本，避免旧 SDK 带的 Volk 与新头文件不匹配。nvpro 第三方代码由其子模块保留原版本/许可证；未开启 nvslang、Aftermath、CUDA、MCP 等不需要的模块。

根 LICENSE 为 Apache-2.0，根 NOTICE 保留 nvpro 示例结构来源说明。OpenGL 的详细第三方许可证见其 THIRD_PARTY.md，素材见 OpenGL/assets/LICENSE.txt。MIT、BSD、Apache、CC0 等各依赖保持原条款，不能因为外层代码采用 Apache 就删掉第三方声明。

压缩包哈希写在各 CMakeLists/Dependencies.cmake，便于 fresh clone 验证下载内容。SDK 的二进制工具、构建产物、日志、备份和本机认证信息不进入 Git 历史。


## 54 课扩展的资产依赖

原生 Vulkan 和 DX12 新实验也使用已固定的 Assimp 5.4.3（只启用 OBJ/glTF）及 stb_image。预下载脚本的对应 Track 会准备 Assimp；构建把课程自制 assets 复制到 exe 旁，运行不依赖工作目录。没有新增需下载的外部商业模型或图片。
