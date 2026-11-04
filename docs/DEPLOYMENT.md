# 部署与打包

## 首次配置

1. 安装 Git、CMake 3.24+、Visual Studio 2022 C++ 桌面工作负载和 Windows SDK。
2. 安装 Vulkan SDK；重新打开终端，确认 VULKAN_SDK 和 glslc。驱动实际支持的 API 版本由 vulkaninfo/程序查询。
3. `git clone --recurse-submodules https://github.com/13536309143/graphics-api-tutorials.git`，进入仓库。
4. `powershell -ExecutionPolicy Bypass -File .scriptsuild.ps1 -Track all -Test`。此策略只针对当前进程。

按需构建时将 Track 换为 opengl / vulkan-native / vulkan-nvpro / dx12 / launcher。构建脚本固定 parallel=4，避免 nvpro/Assimp 大量并行占满内存。

## 根 preset 与独立子工程

根 CMakePresets.json 是推荐入口：为四条路线和启动器各用独立 binaryDir。也可从某路线目录执行 `cmake -S . -B build-local -G "Visual Studio 17 2022" -A x64`，不要把独立子工程配置写入已经由根 preset 创建的缓存。切换生成器/source root 时换一个新 build 目录。

## 首次联网与离线

nvpro_core2 为固定子模块；GLFW/GLM/Assimp、Vulkan 辅助头文件使用固定版本、压缩包 SHA256 拉取。构建产物和缓存不提交 Git。网络受限时可在联网机器先完成 configure，再保留相应 build/_deps 缓存及 nvpro 子模块；OpenGL 也接受 third_party 下已有的对应源码树。复制缓存到另一条绝对路径后，不能直接复用旧 CMakeCache，请重新配置。

日常运行不联网。Vulkan 原生和 nvpro 的 SPIR-V 位于 exe 旁的 shaders/；DX12 的 HLSL 位于相同相对目录；OpenGL 的 GLSL/assets 同样随构建复制。程序应从最终部署文件运行，不能只拷贝 exe 丢掉资源。

## 发布包

```powershell
cmake --preset dx12
cmake --build --preset dx12-release --parallel 4
powershell -ExecutionPolicy Bypass -File .scriptspackage.ps1 -Track dx12
```

dist/ 下生成该路线的 Windows x64 ZIP。其他路线替换 Track。打包脚本打包整个 Release/bin 内容；当前使用的 nvpro 子库为静态库，OpenGL 依赖也静态链接。目标机器仍需显卡驱动和 Microsoft Visual C++ 运行库；Debug CRT 不适合对外分发，因此推荐 Release。

整个学习入口需要源码/HTML/目录结构与启动器；路线 ZIP 用来运行示例，不包含完整阅读站点。没有自动把网站发布到外网，阅读器只绑定 127.0.0.1。服务关闭后重新双击开始学习.cmd 即可恢复。


## 备用预下载入口

若 CMake 的 CDN 下载停滞，可先从根目录执行 `powershell -ExecutionPolicy Bypass -File .\scripts\prepare-dependencies.ps1 -Track all`，再运行 build。该脚本通过 Windows 下载栈获取官方归档，逐一校验 SHA256 并保存到 Git 忽略的源码缓存；也可 `deploy.ps1 -PrepareCache -Build` 一步处理。它不关闭 TLS 证书校验，不改变系统 SDK。单条路线可将 Track 换成对应名称。
