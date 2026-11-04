# 依赖与许可证

| 依赖 | 固定版本 | 本地路径 | 用途 |
| --- | --- | --- | --- |
| GLFW | 3.4 | third_party/glfw | 窗口、上下文、输入 |
| GLM | 1.0.1 | third_party/glm | 向量与矩阵；仅引用头文件 |
| GLAD generator | 2.0.8 | third_party/glad | 已生成 OpenGL 3.3 Core，扩展列表为空 |

来源：
- https://github.com/glfw/glfw/releases/tag/3.4
- https://github.com/g-truc/glm/releases/tag/1.0.1
- https://github.com/Dav1dde/glad/releases/tag/v2.0.8

GLAD 生成命令：`python -m glad --reproducible --api gl:core=3.3 --extensions "" --out-path third_party/glad c`。已包含生成结果，日常构建不需要 Python 或联网。生成器使用发行版内置的规范文件以保证可重现。

GLFW 的许可证在 third_party/glfw/LICENSE.md；GLM 的许可证在 third_party/glm/copying.txt；GLAD 许可证在 third_party/glad/LICENSE，生成头文件还包含其上游规范声明。保留这些文件再分发。

源码压缩包的 SHA256 另存于 DEPENDENCIES.sha256。基础课程代码和中文说明可用于个人学习、修改与练习；第三方内容继续遵循各自许可证。


## 扩充依赖

- Assimp 5.4.3：[官方源码](https://github.com/assimp/assimp/releases/tag/v5.4.3)，目录 third_party/assimp，许可证 LICENSE。静态构建，仅开启 OBJ/GLTF（含 GLB）导入，附带 zlib 等传递依赖，保留原许可证。
- stb_image：固定提交 `d14d46f6b44456eb88beac58cba833a4135d4c89`，[源码](https://github.com/nothings/stb/tree/d14d46f6b44456eb88beac58cba833a4135d4c89)，目录 third_party/stb，许可证 LICENSE。PNG/JPEG/HDR 等由该头文件解码。
- assets 中模型、PNG、HDR 为本课程自制素材，按 [CC0](https://creativecommons.org/publicdomain/zero/1.0/) 提供，声明在 assets/LICENSE.txt。没有下载第三方模型代替演示。

以上均随工程提供。日常构建无网络请求；Python/Pillow 仅用于制作素材，不是运行/编译教程的依赖。Assimp 5.4.3 的旧自定义目标在 CMake 4 下采用局部默认 CMP0175=OLD 兼容，未改第三方源码。
